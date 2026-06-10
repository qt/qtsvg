// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qsvgnode_p.h"
#include "qsvgdocument_p.h"
#include "qsvggraphics_p.h"
#include <QtSvg/private/qsvgpaintserver_p.h>

#include <QLoggingCategory>
#include<QElapsedTimer>
#include <QtGui/qimageiohandler.h>

#include "qdebug.h"

#include <QtGui/private/qoutlinemapper_p.h>

#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifndef QT_NO_DEBUG
Q_STATIC_LOGGING_CATEGORY(lcSvgTiming, "qt.svg.timing")
#endif

#if !defined(QT_SVG_SIZE_LIMIT)
#  define QT_SVG_SIZE_LIMIT QT_RASTER_COORD_LIMIT
#endif

QSvgNode::QSvgNode(QSvgNode *parent)
    : m_parent(parent),
      m_displayMode(BlockMode),
      m_visible(true)
{
}

QSvgNode::~QSvgNode()
{

}

void QSvgNode::draw(QPainter *p, QSvgExtraStates &states)
{
#ifndef QT_NO_DEBUG
    QElapsedTimer qtSvgTimer; qtSvgTimer.start();
#endif

    if (shouldDrawNode(p, states)) {
        quint8 remainingDepth = states.trustedSource ? states.remainingNestedNodes
                                                     : states.remainingNestedNodes - 1;
        QScopedValueRollback<quint8> nestedNodesGuard(states.remainingNestedNodes, remainingDepth);
        if (states.remainingNestedNodes == 0) {
            qCWarning(lcSvgDraw).noquote() << "Too many nested nodes at" << typeName()
                                 << "exceeding max nested limit of" << QtSvg::renderingMaxNestedNodes << "."
                                 << "Enable AssumeTrustedSource in QSvgHandler or set QT_SVG_DEFAULT_OPTIONS=2 to disable this check.";
            return;
        }

        applyStyle(p, states);
        applyAnimatedStyle(p, states);
        QSvgNode *maskNode = this->hasMask() ? document()->namedNode(this->maskId()) : nullptr;
        QSvgFilterContainer *filterNode = this->hasFilter() ? static_cast<QSvgFilterContainer*>(document()->namedNode(this->filterId()))
                                                            : nullptr;
        if (filterNode && filterNode->type() == QSvgNode::Filter && filterNode->supported()) {
            QTransform xf = p->transform();
            p->resetTransform();
            QRectF localRect = internalBounds(p, states);
            p->setTransform(xf);
            QRectF boundsRect = xf.mapRect(filterNode->filterRegion(localRect));
            QImage proxy = drawIntoBuffer(p, states, boundsRect.toRect());
            proxy = filterNode->applyFilter(proxy, p, localRect);
            if (maskNode && maskNode->type() == QSvgNode::Mask) {
                boundsRect = QRectF(proxy.offset(), proxy.size());
                localRect = p->transform().inverted().mapRect(boundsRect);
                QImage mask = static_cast<QSvgMask*>(maskNode)->createMask(p, states, localRect, &boundsRect);
                applyMaskToBuffer(&proxy, mask);
            }
            applyBufferToCanvas(p, proxy);

        } else if (maskNode && maskNode->type() == QSvgNode::Mask) {
            QRectF boundsRect;
            QImage mask = static_cast<QSvgMask*>(maskNode)->createMask(p, states, this, &boundsRect);
            drawWithMask(p, states, mask, boundsRect.toRect());
        } else if (!qFuzzyCompare(p->opacity(), qreal(1.0)) && requiresGroupRendering()) {
            QTransform xf = p->transform();
            p->resetTransform();

            QRectF localRect = decoratedInternalBounds(p, states);
            // adding safety border needed because of the antialiazing effects
            QRectF boundsRect = xf.mapRect(localRect);
            const int deltaX = boundsRect.width() * 0.1;
            const int deltaY = boundsRect.height() * 0.1;
            boundsRect = boundsRect.adjusted(-deltaX, -deltaY, deltaX, deltaY);

            p->setTransform(xf);

            QImage proxy = drawIntoBuffer(p, states, boundsRect.toAlignedRect());
            applyBufferToCanvas(p, proxy);
        } else {
            if (separateFillStroke(p, states))
                fillThenStroke(p, states);
            else
                drawCommand(p, states);

        }
        revertAnimatedStyle(p ,states);
        revertStyle(p, states);
    }

#ifndef QT_NO_DEBUG
    if (Q_UNLIKELY(lcSvgTiming().isDebugEnabled()))
        qCDebug(lcSvgTiming) << "Drawing" << typeName() << "took" << (qtSvgTimer.nsecsElapsed() / 1000000.0f) << "ms";
#endif
}

void QSvgNode::fillThenStroke(QPainter *p, QSvgExtraStates &states)
{
    qreal oldOpacity = p->opacity();
    if (p->brush().style() != Qt::NoBrush) {
        QPen oldPen = p->pen();
        p->setPen(Qt::NoPen);
        p->setOpacity(oldOpacity * states.fillOpacity);

        drawCommand(p, states);

        p->setPen(oldPen);
    }
    if (p->pen() != Qt::NoPen && p->pen().brush() != Qt::NoBrush && p->pen().widthF() != 0) {
        QBrush oldBrush = p->brush();
        p->setOpacity(oldOpacity * states.strokeOpacity);
        p->setBrush(Qt::NoBrush);

        drawCommand(p, states);

        p->setBrush(oldBrush);
    }
    p->setOpacity(oldOpacity);
}

void QSvgNode::drawWithMask(QPainter *p, QSvgExtraStates &states, const QImage &mask, const QRect &boundsRect)
{
    QImage proxy = drawIntoBuffer(p, states, boundsRect);
    if (proxy.isNull())
        return;
    applyMaskToBuffer(&proxy, mask);

    p->save();
    p->resetTransform();
    p->drawImage(boundsRect, proxy);
    p->restore();
}

QImage QSvgNode::drawIntoBuffer(QPainter *p, QSvgExtraStates &states, const QRect &boundsRect)
{
    QImage proxy;
    if (!QImageIOHandler::allocateImage(boundsRect.size(), QImage::Format_ARGB32_Premultiplied, &proxy)) {
        qCWarning(lcSvgDraw) << "The requested buffer size is too big, ignoring";
        return proxy;
    }
    proxy.setOffset(boundsRect.topLeft());
    proxy.fill(Qt::transparent);
    QPainter proxyPainter(&proxy);
    proxyPainter.setPen(p->pen());
    proxyPainter.setBrush(p->brush());
    proxyPainter.setFont(p->font());
    proxyPainter.translate(-boundsRect.topLeft());
    proxyPainter.setTransform(p->transform(), true);
    proxyPainter.setRenderHints(p->renderHints());
    if (separateFillStroke(p, states))
        fillThenStroke(&proxyPainter, states);
    else
        drawCommand(&proxyPainter, states);
    return proxy;
}

void QSvgNode::applyMaskToBuffer(QImage *proxy, const QImage &mask) const
{
    QPainter proxyPainter(proxy);
    proxyPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    proxyPainter.resetTransform();
    proxyPainter.drawImage(QRect(0, 0, mask.width(), mask.height()), mask);
}

void QSvgNode::applyBufferToCanvas(QPainter *p, const QImage &proxy) const
{
    QTransform xf = p->transform();
    p->resetTransform();
    p->drawImage(QRect(proxy.offset(), proxy.size()), proxy);
    p->setTransform(xf);
}

bool QSvgNode::isDescendantOf(const QSvgNode *parent) const
{
    const QSvgNode *n = this;
    while (n) {
        if (n == parent)
            return true;
        n = n->m_parent;
    }
    return false;
}

void QSvgNode::appendStyleProperty(QSvgStylePropertyPtr prop)
{
    m_style.appendProperty(std::move(prop));
}

void QSvgNode::applyStyle(QPainter *p, QSvgExtraStates &states) const
{
    m_style.apply(p, this, states);
}

/*!
    \internal

    Apply the styles of all parents to the painter and the states.
    The styles are applied from the top level node to the current node.
    This function can be used to set the correct style for a node
    if it's draw function is triggered out of the ordinary draw context,
    for example the mask node, that is cross-referenced.
*/
void QSvgNode::applyStyleRecursive(QPainter *p, QSvgExtraStates &states) const
{
    std::vector<const QSvgNode *> parents;

    const QSvgNode *current = this;
    do {
        parents.push_back(current);
        current = current->parent();
    } while (current);

    for (auto i = parents.crbegin(); i != parents.crend(); ++i)
        (*i)->applyStyle(p, states);
}

void QSvgNode::revertStyle(QPainter *p, QSvgExtraStates &states) const
{
    m_style.revert(p, states);
}

void QSvgNode::revertStyleRecursive(QPainter *p, QSvgExtraStates &states) const
{
    const QSvgNode *current = this;
    do {
        current->revertStyle(p, states);
        current = current->parent();
    } while (current);
}

void QSvgNode::applyAnimatedStyle(QPainter *p, QSvgExtraStates &states) const
{
    if (document()->animated())
        m_animatedStyle.apply(p, this, states);
}

void QSvgNode::revertAnimatedStyle(QPainter *p, QSvgExtraStates &states) const
{
    if (document()->animated())
        m_animatedStyle.revert(p, states);
}

QSvgStyleProperty *QSvgNode::styleProperty(QSvgStyleProperty::Type type) const
{
    const QSvgNode *node = this;
    while (node) {
        if (QSvgStyleProperty *prop = node->m_style.property(type))
            return prop;

        node = node->parent();
    }

    return nullptr;
}

QRectF QSvgNode::internalFastBounds(QPainter *p, QSvgExtraStates &states) const
{
    return internalBounds(p, states);
}

QRectF QSvgNode::internalBounds(QPainter *, QSvgExtraStates &) const
{
    return QRectF(0, 0, 0, 0);
}

QRectF QSvgNode::bounds() const
{
    if (!m_cachedBounds.isEmpty())
        return m_cachedBounds;

    QImage dummy(1, 1, QImage::Format_RGB32);
    QPainter p(&dummy);
    initPainter(&p);
    QSvgExtraStates states;

    if (parent())
        parent()->applyStyleRecursive(&p, states);
    p.setWorldTransform(QTransform());
    m_cachedBounds = bounds(&p, states);
    if (parent()) // always revert the style to not store old transformations
        parent()->revertStyleRecursive(&p, states);
    return m_cachedBounds;
}

QSvgDocument * QSvgNode::document() const
{
    QSvgDocument *doc = nullptr;
    QSvgNode *node = const_cast<QSvgNode*>(this);
    while (node && node->type() != QSvgNode::Doc) {
        node = node->parent();
    }
    doc = static_cast<QSvgDocument*>(node);

    return doc;
}

QLatin1StringView QSvgNode::typeName() const
{
    switch (type()) {
    case Doc: return "svg"_L1;
    case Group: return "g"_L1;
    case Defs: return "defs"_L1;
    case Switch: return "switch"_L1;
    case AnimateColor: return "animateColor"_L1;
    case AnimateTransform: return "animateTransform"_L1;
    case Circle: return "circle"_L1;
    case Ellipse: return "ellipse"_L1;
    case Image: return "image"_L1;
    case Line: return "line"_L1;
    case Path: return "path"_L1;
    case Polygon: return "polygon"_L1;
    case Polyline: return "polyline"_L1;
    case Rect: return "rect"_L1;
    case Text: return "text"_L1;
    case Textarea: return "textarea"_L1;
    case Tspan: return "tspan"_L1;
    case Use: return "use"_L1;
    case Video: return "video"_L1;
    case Mask: return "mask"_L1;
    case Symbol: return "symbol"_L1;
    case Marker: return "marker"_L1;
    case Pattern: return "pattern"_L1;
    case Filter: return "filter"_L1;
    case FeMerge: return "feMerge"_L1;
    case FeMergenode: return "feMergeNode"_L1;
    case FeColormatrix: return "feColorMatrix"_L1;
    case FeGaussianblur: return "feGaussianBlur"_L1;
    case FeOffset: return "feOffset"_L1;
    case FeComposite: return "feComposite"_L1;
    case FeFlood: return "feFlood"_L1;
    case FeBlend: return "feBlend"_L1;
    case FeUnsupported: return "feUnsupported"_L1;
    case Font: return "font"_L1;
    }
    return "unknown"_L1;
}

void QSvgNode::setRequiredFeatures(const QStringList &lst)
{
    m_requiredFeatures = lst;
}

const QStringList & QSvgNode::requiredFeatures() const
{
    return m_requiredFeatures;
}

void QSvgNode::setRequiredExtensions(const QStringList &lst)
{
    m_requiredExtensions = lst;
}

const QStringList & QSvgNode::requiredExtensions() const
{
    return m_requiredExtensions;
}

void QSvgNode::setRequiredLanguages(const QStringList &lst)
{
    m_requiredLanguages = lst;
}

const QStringList & QSvgNode::requiredLanguages() const
{
    return m_requiredLanguages;
}

void QSvgNode::setRequiredFormats(const QStringList &lst)
{
    m_requiredFormats = lst;
}

const QStringList & QSvgNode::requiredFormats() const
{
    return m_requiredFormats;
}

void QSvgNode::setRequiredFonts(const QStringList &lst)
{
    m_requiredFonts = lst;
}

const QStringList & QSvgNode::requiredFonts() const
{
    return m_requiredFonts;
}

void QSvgNode::setVisible(bool visible)
{
    //propagate visibility change of true to the parent
    //not propagating false is just a small performance
    //degradation since we'll iterate over children without
    //drawing any of them
    if (m_parent && visible && !m_parent->isVisible())
        m_parent->setVisible(true);

    m_visible = visible;
}

QRectF QSvgNode::bounds(QPainter *p, QSvgExtraStates &states) const
{
    applyStyle(p, states);
    QRectF rect = internalBounds(p, states);
    revertStyle(p, states);
    return rect;
}

QRectF QSvgNode::decoratedInternalBounds(QPainter *p, QSvgExtraStates &states) const
{
    return filterRegion(internalBounds(p, states));
}

QRectF QSvgNode::decoratedBounds(QPainter *p, QSvgExtraStates &states) const
{
    applyStyle(p, states);
    QRectF rect = decoratedInternalBounds(p, states);
    revertStyle(p, states);
    return rect;
}

void QSvgNode::setNodeId(const QString &i)
{
    m_id = i;
}

void QSvgNode::setXmlClass(const QString &str)
{
    m_class = str;
}

QString QSvgNode::maskId() const
{
    return m_maskId;
}

void QSvgNode::setMaskId(const QString &str)
{
    m_maskId = str;
}

bool QSvgNode::hasMask() const
{
    return !m_maskId.isEmpty();
}

QString QSvgNode::filterId() const
{
    return m_filterId;
}

void QSvgNode::setFilterId(const QString &str)
{
    m_filterId = str;
}

bool QSvgNode::hasFilter() const
{
    return !m_filterId.isEmpty();
}

QString QSvgNode::markerStartId() const
{
    return m_markerStartId;
}

void QSvgNode::setMarkerStartId(const QString &str)
{
    m_markerStartId = str;
}

bool QSvgNode::hasMarkerStart() const
{
    return !m_markerStartId.isEmpty();
}

QString QSvgNode::markerMidId() const
{
    return m_markerMidId;
}

void QSvgNode::setMarkerMidId(const QString &str)
{
    m_markerMidId = str;
}

bool QSvgNode::hasMarkerMid() const
{
    return !m_markerMidId.isEmpty();
}

QString QSvgNode::markerEndId() const
{
    return m_markerEndId;
}

void QSvgNode::setMarkerEndId(const QString &str)
{
    m_markerEndId = str;
}

bool QSvgNode::hasMarkerEnd() const
{
    return !m_markerEndId.isEmpty();
}

bool QSvgNode::hasAnyMarker() const
{
    return hasMarkerStart() || hasMarkerMid() || hasMarkerEnd();
}

bool QSvgNode::requiresGroupRendering() const
{
    return false;
}

void QSvgNode::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
}

QSvgNode::DisplayMode QSvgNode::displayMode() const
{
    return m_displayMode;
}

qreal QSvgNode::strokeWidth(QPainter *p)
{
    const QPen &pen = p->pen();
    if (pen.style() == Qt::NoPen || pen.brush().style() == Qt::NoBrush || pen.isCosmetic())
        return 0;
    return pen.widthF();
}

void QSvgNode::initPainter(QPainter *p)
{
    QPen pen(Qt::NoBrush, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
    pen.setMiterLimit(4);
    p->setPen(pen);
    p->setBrush(Qt::black);
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    QFont font(p->font());
    if (font.pointSize() < 0 && font.pixelSize() > 0) {
        font.setPointSizeF(font.pixelSize() * 72.0 / p->device()->logicalDpiY());
        p->setFont(font);
    }
}

QRectF QSvgNode::boundsOnStroke(QPainter *p, const QPainterPath &path,
                                qreal width, BoundsMode mode)
{
    QPainterPathStroker stroker;
    stroker.setWidth(width);
    if (mode == BoundsMode::IncludeMiterLimit) {
        stroker.setJoinStyle(p->pen().joinStyle());
        stroker.setMiterLimit(p->pen().miterLimit());
    }
    QPainterPath stroke = stroker.createStroke(path);
    return p->transform().map(stroke).boundingRect();
}

bool QSvgNode::shouldDrawNode(QPainter *p, QSvgExtraStates &states) const
{
    if (m_displayMode == DisplayMode::NoneMode)
        return false;

    if (document() && states.trustedSource)
        return true;

    QRectF brect = internalFastBounds(p, states);
    if (brect.width() <= QT_SVG_SIZE_LIMIT && brect.height() <= QT_SVG_SIZE_LIMIT) {
        return true;
    } else {
        qCWarning(lcSvgDraw) << "Shape of type" << type() << "ignored because it will take too long to rasterize (bounding rect=" << brect << ")."
                             << "Enable AssumeTrustedSource in QSvgHandler or set QT_SVG_DEFAULT_OPTIONS=2 to disable this check.";
        return false;
    }
}

QRectF QSvgNode::filterRegion(QRectF bounds) const
{
    QSvgFilterContainer *filterNode = hasFilter()
            ? static_cast<QSvgFilterContainer*>(document()->namedNode(filterId()))
            : nullptr;

    if (filterNode && filterNode->type() == QSvgNode::Filter && filterNode->supported())
        return filterNode->filterRegion(bounds);

    return bounds;
}

QT_END_NAMESPACE
