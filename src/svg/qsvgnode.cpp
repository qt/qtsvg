/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt SVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qsvgnode_p.h"
#include "qsvgtinydocument_p.h"

#include <QLoggingCategory>

#include "qdebug.h"
#include "qstack.h"

#include <QtGui/private/qoutlinemapper_p.h>

#if !defined(QT_SVG_SIZE_LIMIT)
#  define QT_SVG_SIZE_LIMIT QT_RASTER_COORD_LIMIT
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSvgDraw)

QSvgNode::QSvgNode(QSvgNode *parent)
    : m_parent(parent),
      m_visible(true),
      m_displayMode(BlockMode)
{
}

QSvgNode::~QSvgNode()
{

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

void QSvgNode::appendStyleProperty(QSvgStyleProperty *prop, const QString &id)
{
    //qDebug()<<"appending "<<prop->type()<< " ("<< id <<") "<<"to "<<this<<this->type();
    QSvgTinyDocument *doc;
    switch (prop->type()) {
    case QSvgStyleProperty::QUALITY:
        m_style.quality = static_cast<QSvgQualityStyle*>(prop);
        break;
    case QSvgStyleProperty::FILL:
        m_style.fill = static_cast<QSvgFillStyle*>(prop);
        break;
    case QSvgStyleProperty::VIEWPORT_FILL:
        m_style.viewportFill = static_cast<QSvgViewportFillStyle*>(prop);
        break;
    case QSvgStyleProperty::FONT:
        m_style.font = static_cast<QSvgFontStyle*>(prop);
        break;
    case QSvgStyleProperty::STROKE:
        m_style.stroke = static_cast<QSvgStrokeStyle*>(prop);
        break;
    case QSvgStyleProperty::SOLID_COLOR:
        m_style.solidColor = static_cast<QSvgSolidColorStyle*>(prop);
        doc = document();
        if (doc && !id.isEmpty())
            doc->addNamedStyle(id, m_style.solidColor);
        break;
    case QSvgStyleProperty::GRADIENT:
        m_style.gradient = static_cast<QSvgGradientStyle*>(prop);
        doc = document();
        if (doc && !id.isEmpty())
            doc->addNamedStyle(id, m_style.gradient);
        break;
    case QSvgStyleProperty::TRANSFORM:
        m_style.transform = static_cast<QSvgTransformStyle*>(prop);
        break;
    case QSvgStyleProperty::ANIMATE_COLOR:
        m_style.animateColor = static_cast<QSvgAnimateColor*>(prop);
        break;
    case QSvgStyleProperty::ANIMATE_TRANSFORM:
        m_style.animateTransforms.append(
            static_cast<QSvgAnimateTransform*>(prop));
        break;
    case QSvgStyleProperty::OPACITY:
        m_style.opacity = static_cast<QSvgOpacityStyle*>(prop);
        break;
    case QSvgStyleProperty::COMP_OP:
        m_style.compop = static_cast<QSvgCompOpStyle*>(prop);
        break;
    default:
        qDebug("QSvgNode: Trying to append unknown property!");
        break;
    }
}

void QSvgNode::applyStyle(QPainter *p, QSvgExtraStates &states) const
{
    m_style.apply(p, this, states);
}

void QSvgNode::revertStyle(QPainter *p, QSvgExtraStates &states) const
{
    m_style.revert(p, states);
}

QSvgStyleProperty * QSvgNode::styleProperty(QSvgStyleProperty::Type type) const
{
    const QSvgNode *node = this;
    while (node) {
        switch (type) {
        case QSvgStyleProperty::QUALITY:
            if (node->m_style.quality)
                return node->m_style.quality;
            break;
        case QSvgStyleProperty::FILL:
            if (node->m_style.fill)
                return node->m_style.fill;
            break;
        case QSvgStyleProperty::VIEWPORT_FILL:
            if (m_style.viewportFill)
                return node->m_style.viewportFill;
            break;
        case QSvgStyleProperty::FONT:
            if (node->m_style.font)
                return node->m_style.font;
            break;
        case QSvgStyleProperty::STROKE:
            if (node->m_style.stroke)
                return node->m_style.stroke;
            break;
        case QSvgStyleProperty::SOLID_COLOR:
            if (node->m_style.solidColor)
                return node->m_style.solidColor;
            break;
        case QSvgStyleProperty::GRADIENT:
            if (node->m_style.gradient)
                return node->m_style.gradient;
            break;
        case QSvgStyleProperty::TRANSFORM:
            if (node->m_style.transform)
                return node->m_style.transform;
            break;
        case QSvgStyleProperty::ANIMATE_COLOR:
            if (node->m_style.animateColor)
                return node->m_style.animateColor;
            break;
        case QSvgStyleProperty::ANIMATE_TRANSFORM:
            if (!node->m_style.animateTransforms.isEmpty())
                return node->m_style.animateTransforms.first();
            break;
        case QSvgStyleProperty::OPACITY:
            if (node->m_style.opacity)
                return node->m_style.opacity;
            break;
        case QSvgStyleProperty::COMP_OP:
            if (node->m_style.compop)
                return node->m_style.compop;
            break;
        default:
            break;
        }
        node = node->parent();
    }

    return 0;
}

QSvgFillStyleProperty * QSvgNode::styleProperty(const QString &id) const
{
    QString rid = id;
    if (rid.startsWith(QLatin1Char('#')))
        rid.remove(0, 1);
    QSvgTinyDocument *doc = document();
    return doc ? doc->namedStyle(rid) : 0;
}

QRectF QSvgNode::fastBounds(QPainter *p, QSvgExtraStates &states) const
{
    return bounds(p, states);
}

QRectF QSvgNode::bounds(QPainter *, QSvgExtraStates &) const
{
    return QRectF(0, 0, 0, 0);
}

QRectF QSvgNode::transformedBounds() const
{
    if (!m_cachedBounds.isEmpty())
        return m_cachedBounds;

    QImage dummy(1, 1, QImage::Format_RGB32);
    QPainter p(&dummy);
    QSvgExtraStates states;

    QPen pen(Qt::NoBrush, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
    pen.setMiterLimit(4);
    p.setPen(pen);

    QStack<QSvgNode*> parentApplyStack;
    QSvgNode *parent = m_parent;
    while (parent) {
        parentApplyStack.push(parent);
        parent = parent->parent();
    }

    for (int i = parentApplyStack.size() - 1; i >= 0; --i)
        parentApplyStack[i]->applyStyle(&p, states);
    
    p.setWorldTransform(QTransform());

    m_cachedBounds = transformedBounds(&p, states);
    return m_cachedBounds;
}

QSvgTinyDocument * QSvgNode::document() const
{
    QSvgTinyDocument *doc = 0;
    QSvgNode *node = const_cast<QSvgNode*>(this);
    while (node && node->type() != QSvgNode::DOC) {
        node = node->parent();
    }
    doc = static_cast<QSvgTinyDocument*>(node);

    return doc;
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

QRectF QSvgNode::transformedBounds(QPainter *p, QSvgExtraStates &states) const
{
    applyStyle(p, states);
    QRectF rect = bounds(p, states);
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

bool QSvgNode::shouldDrawNode(QPainter *p, QSvgExtraStates &states) const
{
    static bool alwaysDraw = qEnvironmentVariableIntValue("QT_SVG_DISABLE_SIZE_LIMIT");
    if (alwaysDraw)
        return true;

    QRectF brect = fastBounds(p, states);
    if (brect.width() <= QT_SVG_SIZE_LIMIT && brect.height() <= QT_SVG_SIZE_LIMIT) {
        return true;
    } else {
        qCWarning(lcSvgDraw) << "Shape of type" << type() << "ignored because it will take too long to rasterize (bounding rect=" << brect << ")."
                             << "Set QT_SVG_DISABLE_SIZE_LIMIT=1 to disable this check.";
        return false;
    }
}

QT_END_NAMESPACE
