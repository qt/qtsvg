// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgstructure_p.h"
#include "qsvggraphics_p.h"

#include "qsvgstyle_p.h"
#include "qsvgtinydocument_p.h"
#include "qsvggraphics_p.h"
#include "qsvgstyle_p.h"
#include "qsvgfilter_p.h"

#include "qpainter.h"
#include "qlocale.h"
#include "qdebug.h"

#include <QLoggingCategory>
#include <qscopedvaluerollback.h>
#include <QtGui/qimageiohandler.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSvgDraw);

QSvgG::QSvgG(QSvgNode *parent)
    : QSvgStructureNode(parent)
{

}

QSvgStructureNode::~QSvgStructureNode()
{
    qDeleteAll(m_renderers);
}

void QSvgG::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    QList<QSvgNode*>::iterator itr = m_renderers.begin();
    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if ((node->isVisible()) && (node->displayMode() != QSvgNode::NoneMode))
            node->draw(p, states);
        ++itr;
    }
}

bool QSvgG::shouldDrawNode(QPainter *, QSvgExtraStates &) const
{
    return true;
}

QSvgNode::Type QSvgG::type() const
{
    return Group;
}

QSvgStructureNode::QSvgStructureNode(QSvgNode *parent)
    :QSvgNode(parent)
{

}

QSvgNode * QSvgStructureNode::scopeNode(const QString &id) const
{
    QSvgTinyDocument *doc = document();
    return doc ? doc->namedNode(id) : 0;
}

void QSvgStructureNode::addChild(QSvgNode *child, const QString &id)
{
    m_renderers.append(child);

    if (id.isEmpty())
        return; //we can't add it to scope without id

    QSvgTinyDocument *doc = document();
    if (doc)
        doc->addNamedNode(id, child);
}

QSvgDefs::QSvgDefs(QSvgNode *parent)
    : QSvgStructureNode(parent)
{
}

bool QSvgDefs::shouldDrawNode(QPainter *, QSvgExtraStates &) const
{
    return false;
}

QSvgNode::Type QSvgDefs::type() const
{
    return Defs;
}

QSvgSymbolLike::QSvgSymbolLike(QSvgNode *parent, QRectF bounds, QRectF viewBox, QPointF refP,
               QSvgSymbolLike::PreserveAspectRatios pAspectRatios, QSvgSymbolLike::Overflow overflow)
    : QSvgStructureNode(parent)
    , m_rect(bounds)
    , m_viewBox(viewBox)
    , m_refP(refP)
    , m_pAspectRatios(pAspectRatios)
    , m_overflow(overflow)
{

}

void QSvgSymbolLike::setPainterToRectAndAdjustment(QPainter *p) const
{
    qreal scaleX = 1;
    if (m_rect.width() > 0 && m_viewBox.width() > 0)
        scaleX = m_rect.width()/m_viewBox.width();
    qreal scaleY = 1;
    if (m_rect.height() > 0 && m_viewBox.height() > 0)
        scaleY = m_rect.height()/m_viewBox.height();

    if (m_overflow == Overflow::Hidden) {
        QTransform t;
        t.translate(- m_refP.x() * scaleX - m_rect.left() - m_viewBox.left() * scaleX,
                    - m_refP.y() * scaleY - m_rect.top() - m_viewBox.top() * scaleY);
        t.scale(scaleX, scaleY);

        if (m_viewBox.isValid())
            p->setClipRect(t.mapRect(m_viewBox));
    }

    qreal offsetX = 0;
    qreal offsetY = 0;

    if (!qFuzzyCompare(scaleX, scaleY) &&
        m_pAspectRatios.testAnyFlag(PreserveAspectRatio::xyMask)) {

        if (m_pAspectRatios.testAnyFlag(PreserveAspectRatio::meet))
            scaleX = scaleY = qMin(scaleX, scaleY);
        else
            scaleX = scaleY = qMax(scaleX, scaleY);

        qreal xOverflow = scaleX * m_viewBox.width() - m_rect.width();
        qreal yOverflow = scaleY * m_viewBox.height() - m_rect.height();

        if ((m_pAspectRatios & PreserveAspectRatio::xMask) == PreserveAspectRatio::xMid)
            offsetX -= xOverflow / 2.;
        else if ((m_pAspectRatios & PreserveAspectRatio::xMask) == PreserveAspectRatio::xMax)
            offsetX -= xOverflow;

        if ((m_pAspectRatios & PreserveAspectRatio::yMask) == PreserveAspectRatio::yMid)
            offsetY -= yOverflow / 2.;
        else if ((m_pAspectRatios & PreserveAspectRatio::yMask) == PreserveAspectRatio::yMax)
            offsetY -= yOverflow;
    }

    p->translate(offsetX - m_refP.x() * scaleX, offsetY - m_refP.y() * scaleY);
    p->scale(scaleX, scaleY);
}

QSvgSymbol::QSvgSymbol(QSvgNode *parent, QRectF bounds, QRectF viewBox, QPointF refP,
                       QSvgSymbol::PreserveAspectRatios pAspectRatios,
                       QSvgSymbol::Overflow overflow)
    : QSvgSymbolLike(parent, bounds, viewBox, refP, pAspectRatios, overflow)
{
}

void QSvgSymbol::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    if (!states.inUse) //Symbol is only drawn when within a use node.
        return;

    QList<QSvgNode*>::iterator itr = m_renderers.begin();

    p->save();
    setPainterToRectAndAdjustment(p);
    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if ((node->isVisible()) && (node->displayMode() != QSvgNode::NoneMode))
            node->draw(p, states);
        ++itr;
    }
    p->restore();
}

QSvgNode::Type QSvgSymbol::type() const
{
    return Symbol;
}

QSvgMarker::QSvgMarker(QSvgNode *parent, QRectF bounds, QRectF viewBox, QPointF refP,
                       QSvgSymbol::PreserveAspectRatios pAspectRatios, QSvgSymbol::Overflow overflow,
                       Orientation orientation, qreal orientationAngle, MarkerUnits markerUnits)
    : QSvgSymbolLike(parent, bounds, viewBox, refP, pAspectRatios, overflow)
    , m_orientation(orientation)
    , m_orientationAngle(orientationAngle)
    , m_markerUnits(markerUnits)
{
    // apply the svg standard style
    QSvgFillStyle *fillProp = new QSvgFillStyle();
    fillProp->setBrush(Qt::black);
    appendStyleProperty(fillProp, QStringLiteral(""));

    QSvgStrokeStyle *strokeProp = new QSvgStrokeStyle();
    strokeProp->setMiterLimit(4);
    strokeProp->setWidth(1);
    strokeProp->setLineCap(Qt::FlatCap);
    strokeProp->setLineJoin(Qt::SvgMiterJoin);
    strokeProp->setStroke(Qt::NoBrush);
    appendStyleProperty(strokeProp, QStringLiteral(""));
}

QSvgFilterContainer::QSvgFilterContainer(QSvgNode *parent, const QSvgRectF &bounds,
                                         QtSvg::UnitTypes filterUnits, QtSvg::UnitTypes primitiveUnits)
    : QSvgStructureNode(parent)
    , m_rect(bounds)
    , m_filterUnits(filterUnits)
    , m_primitiveUnits(primitiveUnits)
    , m_supported(true)
{

}

void QSvgMarker::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    if (!states.inUse) //Symbol is only drawn in combination with another node.
        return;

    if (Q_UNLIKELY(m_recursing))
        return;
    QScopedValueRollback<bool> recursingGuard(m_recursing, true);

    QList<QSvgNode*>::iterator itr = m_renderers.begin();

    p->save();
    applyStyle(p, states);

    setPainterToRectAndAdjustment(p);

    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if ((node->isVisible()) && (node->displayMode() != QSvgNode::NoneMode))
            node->draw(p, states);
        ++itr;
    }
    revertStyle(p, states);
    p->restore();
}

void QSvgMarker::drawMarkersForNode(QSvgNode *node, QPainter *p, QSvgExtraStates &states)
{
    QScopedValueRollback<bool> inUseGuard(states.inUse, true);

    auto getMeanAngle = [] (QPointF p0, QPointF p1, QPointF p2) {
        QPointF t1 = p1 - p0;
        QPointF t2 = p2 - p1;
        qreal hyp1 =  hypot(t1.x(), t1.y());
        if (hyp1 > 0)
            t1 /= hyp1;
        else
            return 0.;
        qreal hyp2 =  hypot(t2.x(), t2.y());
        if (hyp2 > 0)
            t2 /= hyp2;
        else
            return 0.;
        QPointF tangent = t1 + t2;
        return -atan2(tangent.y(), tangent.x()) / M_PI * 180.;
    };

    if (node->hasAnyMarker()) {
        QList<PositionMarkerPair> marks;
        if (node->type() == Line) {
            QSvgLine *line = static_cast<QSvgLine*>(node);
            if (!line)
                return;
            if (node->hasMarkerStart())
                marks << PositionMarkerPair { line->line().p1().x(), line->line().p1().y(),
                                              line->line().angle(), line->markerStartId(),
                                              true};
            if (node->hasMarkerEnd())
                marks << PositionMarkerPair { line->line().p2().x(), line->line().p2().y(),
                                              line->line().angle(), line->markerEndId() };
        } else if (node->type() == Polyline || node->type() == Polygon) {
            QSvgPolyline *polyline = static_cast<QSvgPolyline*>(node);
            QSvgPolygon *polygon = static_cast<QSvgPolygon*>(node);

            const QPolygonF &polyData = (node->type() == Polyline) ? polyline->polygon() : polygon->polygon();

            if (node->hasMarkerStart() && polyData.size() > 1) {
                QLineF line(polyData.at(0), polyData.at(1));
                marks << PositionMarkerPair { line.p1().x(),
                                              line.p1().y(),
                                              line.angle(),
                                              node->markerStartId(),
                                              true };
            }
            if (node->hasMarkerMid()) {
                for (int i = 1; i < polyData.size() - 1; i++) {
                    QPointF p0 = polyData.at(i-1);
                    QPointF p1 = polyData.at(i);
                    QPointF p2 = polyData.at(i+1);

                    marks << PositionMarkerPair { p1.x(),
                                                  p1.y(),
                                                  getMeanAngle(p0, p1, p2),
                                                  node->markerStartId() };
                }
            }
            if (node->hasMarkerEnd() && polyData.size() > 1) {
                QLineF line(polyData.at(polyData.size()-1), polyData.last());
                marks << PositionMarkerPair { line.p2().x(),
                                              line.p2().y(),
                                              line.angle(),
                                              node->markerEndId() };
            }
        } else if (node->type() == Path) {
            QSvgPath *path = static_cast<QSvgPath*>(node);
            if (!path)
                return;
            if (node->hasMarkerStart())
                marks << PositionMarkerPair { path->path().pointAtPercent(0.).x(),
                                              path->path().pointAtPercent(0.).y(),
                                              path->path().angleAtPercent(0.),
                                              path->markerStartId(),
                                              true };
            if (node->hasMarkerMid()) {
                for (int i = 1; i < path->path().elementCount() - 1; i++) {
                    if (path->path().elementAt(i).type == QPainterPath::MoveToElement)
                        continue;
                    if (path->path().elementAt(i).type == QPainterPath::CurveToElement)
                        continue;
                    if (( path->path().elementAt(i).type == QPainterPath::CurveToDataElement &&
                        path->path().elementAt(i+1).type != QPainterPath::CurveToDataElement ) ||
                        path->path().elementAt(i).type == QPainterPath::LineToElement) {

                        QPointF p0(path->path().elementAt(i-1).x, path->path().elementAt(i-1).y);
                        QPointF p1(path->path().elementAt(i).x, path->path().elementAt(i).y);
                        QPointF p2(path->path().elementAt(i+1).x, path->path().elementAt(i+1).y);

                        marks << PositionMarkerPair { p1.x(),
                                                      p1.y(),
                                                      getMeanAngle(p0, p1, p2),
                                                      path->markerMidId() };
                    }
                }
            }
            if (node->hasMarkerEnd())
                marks << PositionMarkerPair { path->path().pointAtPercent(1.).x(),
                                              path->path().pointAtPercent(1.).y(),
                                              path->path().angleAtPercent(1.),
                                              path->markerEndId() };
        }
        for (auto &i : marks) {
            QSvgMarker *markNode = static_cast<QSvgMarker*>(node->document()->namedNode(i.markerId));
            if (!markNode) {
                continue;
            }
            p->save();
            p->translate(i.x, i.y);
            if (markNode->orientation() == QSvgMarker::Orientation::Value)
                p->rotate(markNode->orientationAngle());
            else {
                p->rotate(-i.angle);
                if (i.isStartNode && markNode->orientation() == QSvgMarker::Orientation::AutoStartReverse)
                    p->rotate(-180);
            }
            QRectF oldRect = markNode->m_rect;
            if (markNode->markerUnits() == QSvgMarker::MarkerUnits::StrokeWidth) {
                markNode->m_rect.setWidth(markNode->m_rect.width() * p->pen().widthF());
                markNode->m_rect.setHeight(markNode->m_rect.height() * p->pen().widthF());
            }
            markNode->draw(p, states);
            markNode->m_rect = oldRect;
            p->restore();
        }
    }
}

QSvgNode::Type QSvgMarker::type() const
{
    return Marker;
}

QImage QSvgFilterContainer::applyFilter(QSvgNode *item, const QImage &buffer, QPainter *p, QRectF bounds) const
{
    QRectF filterBounds = m_rect.combinedWithLocalRect(bounds, document()->viewBox(), m_filterUnits);
    QRect filterBoundsGlob = p->transform().mapRect(filterBounds).toRect();
    QRect filterBoundsGlobRel = filterBoundsGlob.translated(-buffer.offset());

    if (filterBoundsGlobRel.isEmpty())
        return buffer;

    QImage proxy;
    if (!QImageIOHandler::allocateImage(filterBoundsGlobRel.size(), buffer.format(), &proxy)) {
        qCWarning(lcSvgDraw) << "The requested filter is too big, ignoring";
        return buffer;
    }
    proxy = buffer.copy(filterBoundsGlobRel);
    proxy.setOffset(filterBoundsGlob.topLeft());
    if (proxy.isNull())
        return buffer;

    QMap<QString, QImage> buffers;
    buffers[QStringLiteral("")] = proxy;
    buffers[QStringLiteral("SourceGraphic")] = proxy;

    bool requiresSourceAlpha = false;

    const QList<QSvgNode *> children = renderers();
    for (const QSvgNode *renderer : children) {
        const QSvgFeFilterPrimitive *filter = QSvgFeFilterPrimitive::castToFilterPrimitive(renderer);
        if (filter && filter->requiresSourceAlpha()) {
            requiresSourceAlpha = true;
            break;
        }
    }

    if (requiresSourceAlpha) {
        QImage proxyAlpha = proxy.convertedTo(QImage::Format_Alpha8).convertedTo(proxy.format());
        proxyAlpha.setOffset(proxy.offset());
        if (proxyAlpha.isNull())
            return buffer;
        buffers[QStringLiteral("SourceAlpha")] = proxyAlpha;
    }

    QImage result;
    for (const QSvgNode *renderer : children) {
        const QSvgFeFilterPrimitive *filter = QSvgFeFilterPrimitive::castToFilterPrimitive(renderer);
        if (filter) {
            result = filter->apply(item, buffers, p, bounds, filterBounds, m_primitiveUnits, m_filterUnits);
            if (!result.isNull()) {
                buffers[QStringLiteral("")] = result;
                buffers[filter->result()] = result;
            }
        }
    }
    return result;
}

void QSvgFilterContainer::setSupported(bool supported)
{
    m_supported = supported;
}

bool QSvgFilterContainer::supported() const
{
    return m_supported;
}

QSvgNode::Type QSvgFilterContainer::type() const
{
    return Filter;
}

/*
  Below is a lookup function based on the gperf output using the following set:

  http://www.w3.org/Graphics/SVG/feature/1.2/#SVG
  http://www.w3.org/Graphics/SVG/feature/1.2/#SVG-static
  http://www.w3.org/Graphics/SVG/feature/1.2/#CoreAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#Structure
  http://www.w3.org/Graphics/SVG/feature/1.2/#ConditionalProcessing
  http://www.w3.org/Graphics/SVG/feature/1.2/#ConditionalProcessingAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#Image
  http://www.w3.org/Graphics/SVG/feature/1.2/#Prefetch
  http://www.w3.org/Graphics/SVG/feature/1.2/#Shape
  http://www.w3.org/Graphics/SVG/feature/1.2/#Text
  http://www.w3.org/Graphics/SVG/feature/1.2/#PaintAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#OpacityAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#GraphicsAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#Gradient
  http://www.w3.org/Graphics/SVG/feature/1.2/#SolidColor
  http://www.w3.org/Graphics/SVG/feature/1.2/#XlinkAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#ExternalResourcesRequiredAttribute
  http://www.w3.org/Graphics/SVG/feature/1.2/#Font
  http://www.w3.org/Graphics/SVG/feature/1.2/#Hyperlinking
  http://www.w3.org/Graphics/SVG/feature/1.2/#Extensibility
*/

// ----- begin of generated code -----

/* C code produced by gperf version 3.0.2 */
/* Command-line: gperf -c -L c svg  */
/* Computed positions: -k'45-46' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

enum {
    TOTAL_KEYWORDS = 20,
    MIN_WORD_LENGTH = 47,
    MAX_WORD_LENGTH = 78,
    MIN_HASH_VALUE = 48,
    MAX_HASH_VALUE = 88
};
/* maximum key range = 41, duplicates = 0 */

inline static bool isSupportedSvgFeature(const QString &str)
{
    static const unsigned char asso_values[] = {
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89,  0, 89,  5,
        15,  5,  0, 10, 89, 89, 89, 89, 89,  0,
        15, 89, 89,  0,  0, 89,  5, 89,  0, 89,
        89, 89, 89, 89, 89, 89, 89,  0, 89, 89,
        89,  0, 89, 89,  0, 89, 89, 89,  0,  5,
        89,  0,  0, 89,  5, 89,  0, 89, 89, 89,
        5,  0, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
        89, 89, 89, 89, 89, 89
    };

    static const char * wordlist[] = {
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Text",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Shape",
        "", "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#SVG",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Structure",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#SolidColor",
        "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Hyperlinking",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#CoreAttribute",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#XlinkAttribute",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#SVG-static",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#OpacityAttribute",
        "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Gradient",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Font",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Image",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#ConditionalProcessing",
        "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Extensibility",
        "", "", "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#GraphicsAttribute",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#Prefetch",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#PaintAttribute",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#ConditionalProcessingAttribute",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "",
        "http://www.w3.org/Graphics/SVG/feature/1.2/#ExternalResourcesRequiredAttribute"
    };

    if (str.size() <= MAX_WORD_LENGTH && str.size() >= MIN_WORD_LENGTH) {
        const char16_t unicode44 = str.at(44).unicode();
        const char16_t unicode45 = str.at(45).unicode();
        if (unicode44 >= sizeof(asso_values) || unicode45 >= sizeof(asso_values))
            return false;
        const int key = str.size()
                        + asso_values[unicode45]
                        + asso_values[unicode44];
        if (key <= MAX_HASH_VALUE && key >= 0)
            return str == QLatin1String(wordlist[key]);
    }
    return false;
}

// ----- end of generated code -----

static inline bool isSupportedSvgExtension(const QString &)
{
    return false;
}


QSvgSwitch::QSvgSwitch(QSvgNode *parent)
    : QSvgStructureNode(parent)
{
    init();
}

void QSvgSwitch::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    QList<QSvgNode*>::iterator itr = m_renderers.begin();

    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if (node->isVisible() && (node->displayMode() != QSvgNode::NoneMode)) {
            const QStringList &features  = node->requiredFeatures();
            const QStringList &extensions = node->requiredExtensions();
            const QStringList &languages = node->requiredLanguages();
            const QStringList &formats = node->requiredFormats();
            const QStringList &fonts = node->requiredFonts();

            bool okToRender = true;
            if (!features.isEmpty()) {
                QStringList::const_iterator sitr = features.constBegin();
                for (; sitr != features.constEnd(); ++sitr) {
                    if (!isSupportedSvgFeature(*sitr)) {
                        okToRender = false;
                        break;
                    }
                }
            }

            if (okToRender && !extensions.isEmpty()) {
                QStringList::const_iterator sitr = extensions.constBegin();
                for (; sitr != extensions.constEnd(); ++sitr) {
                    if (!isSupportedSvgExtension(*sitr)) {
                        okToRender = false;
                        break;
                    }
                }
            }

            if (okToRender && !languages.isEmpty()) {
                QStringList::const_iterator sitr = languages.constBegin();
                okToRender = false;
                for (; sitr != languages.constEnd(); ++sitr) {
                    if ((*sitr).startsWith(m_systemLanguagePrefix)) {
                        okToRender = true;
                        break;
                    }
                }
            }

            if (okToRender && !formats.isEmpty()) {
                okToRender = false;
            }

            if (okToRender && !fonts.isEmpty()) {
                okToRender = false;
            }

            if (okToRender) {
                node->draw(p, states);
                break;
            }
        }
        ++itr;
    }
}

QSvgNode::Type QSvgSwitch::type() const
{
    return Switch;
}

void QSvgSwitch::init()
{
    QLocale locale;
    m_systemLanguage = locale.name().replace(QLatin1Char('_'), QLatin1Char('-'));
    int idx = m_systemLanguage.indexOf(QLatin1Char('-'));
    m_systemLanguagePrefix = m_systemLanguage.mid(0, idx);
}

QRectF QSvgStructureNode::bounds(QPainter *p, QSvgExtraStates &states) const
{
    QRectF bounds;
    if (!m_recursing) {
        QScopedValueRollback<bool> guard(m_recursing, true);
        for (QSvgNode *node : std::as_const(m_renderers))
            bounds |= node->transformedBounds(p, states);
    }
    return bounds;
}

QSvgNode* QSvgStructureNode::previousSiblingNode(QSvgNode *n) const
{
    QSvgNode *prev = nullptr;
    QList<QSvgNode*>::const_iterator itr = m_renderers.constBegin();
    for (; itr != m_renderers.constEnd(); ++itr) {
        QSvgNode *node = *itr;
        if (node == n)
            return prev;
        prev = node;
    }
    return prev;
}

QSvgMask::QSvgMask(QSvgNode *parent, QSvgRectF bounds,
                   QtSvg::UnitTypes contentUnits)
    : QSvgStructureNode(parent)
    , m_rect(bounds)
    , m_contentUnits(contentUnits)
{
}

QImage QSvgMask::createMask(QPainter *p, QSvgExtraStates &states, QSvgNode *targetNode, QRectF *globalRect) const
{
    QTransform t = p->transform();
    p->resetTransform();
    QRectF basicRect = targetNode->bounds(p, states);
    *globalRect = t.mapRect(basicRect);
    p->setTransform(t);
    return createMask(p, states, basicRect, globalRect);
}

QImage QSvgMask::createMask(QPainter *p, QSvgExtraStates &states, const QRectF &localRect, QRectF *globalRect) const
{
    QRect imageBound = globalRect->toAlignedRect();
    *globalRect = imageBound.toRectF();

    QImage mask;
    if (!QImageIOHandler::allocateImage(imageBound.size(), QImage::Format_RGBA8888, &mask)) {
        qCWarning(lcSvgDraw) << "The requested mask size is too big, ignoring";
        return mask;
    }

    if (Q_UNLIKELY(m_recursing))
        return mask;
    QScopedValueRollback<bool> recursingGuard(m_recursing, true);

    // Chrome seems to return the mask of the mask if a mask is set on the mask
    if (this->hasMask()) {
        QSvgMask *maskNode = static_cast<QSvgMask*>(document()->namedNode(this->maskId()));
        if (maskNode) {
            QRectF boundsRect;
            return maskNode->createMask(p, states, localRect, &boundsRect);
        }
    }

    // The mask is created with other elements during rendering.
    // Black pixels are masked out, white pixels are not masked.
    // The strategy is to draw the elements in a buffer (QImage) and to map
    // the white-black image into a transparent-white image that can be used
    // with QPainters composition mode to set the mask.

    mask.fill(Qt::transparent);
    QPainter painter(&mask);
    initPainter(&painter);

    QSvgExtraStates maskNodeStates;
    applyStyleRecursive(&painter, maskNodeStates);

    // The transformation of the mask node is not relevant. What matters are the contentUnits
    // and the position/scale of the node that the mask is applied to.
    painter.resetTransform();
    painter.translate(-imageBound.topLeft());
    painter.setTransform(p->transform(), true);

    QTransform oldT = painter.transform();
    if (m_contentUnits == QtSvg::UnitTypes::objectBoundingBox){
        painter.translate(localRect.topLeft());
        painter.scale(localRect.width(), localRect.height());
    }

    // Draw all content items of the mask to generate the mask
    QList<QSvgNode*>::const_iterator itr = m_renderers.begin();
    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if ((node->isVisible()) && (node->displayMode() != QSvgNode::NoneMode))
            node->draw(&painter, maskNodeStates);
        ++itr;
    }

    for (int i=0; i < mask.height(); i++) {
        QRgb *line = reinterpret_cast<QRgb *>(mask.scanLine(i));
        for (int j=0; j < mask.width(); j++) {
            const qreal rC = 0.2125, gC = 0.7154, bC = 0.0721; //luminanceToAlpha times alpha following SVG 1.1
            int alpha = 255 - (qRed(line[j]) * rC + qGreen(line[j]) * gC + qBlue(line[j]) * bC) * qAlpha(line[j])/255.;
            line[j] = qRgba(0, 0, 0, alpha);
        }
    }

    // Make a path out of the clipRectangle and draw it inverted - black over all content items.
    // This is required to apply a clip rectangle with transformations.
    // painter.setClipRect(clipRect) sounds like the obvious thing to do but
    // created artifacts due to antialiasing.
    QRectF clipRect = m_rect.combinedWithLocalRect(localRect);
    QPainterPath clipPath;
    clipPath.setFillRule(Qt::OddEvenFill);
    clipPath.addRect(mask.rect().adjusted(-10, -10, 20, 20));
    clipPath.addPolygon(oldT.map(QPolygonF(clipRect)));
    painter.resetTransform();
    painter.fillPath(clipPath, Qt::black);

    return mask;
}

QSvgNode::Type QSvgMask::type() const
{
    return Mask;
}

QSvgPattern::QSvgPattern(QSvgNode *parent, QSvgRectF bounds, QRectF viewBox,
                         QtSvg::UnitTypes contentUnits, QTransform transform)
    : QSvgStructureNode(parent),
    m_rect(bounds),
    m_viewBox(viewBox),
    m_contentUnits(contentUnits),
    m_transform(transform)

{

}

static QImage& defaultPattern()
{
    static QImage checkerPattern;

    if (checkerPattern.isNull()) {
        checkerPattern = QImage(QSize(8, 8), QImage::Format_ARGB32);
        QPainter p(&checkerPattern);
        p.fillRect(QRect(0, 0, 4, 4), QColorConstants::Svg::white);
        p.fillRect(QRect(4, 0, 4, 4), QColorConstants::Svg::black);
        p.fillRect(QRect(0, 4, 4, 4), QColorConstants::Svg::black);
        p.fillRect(QRect(4, 4, 4, 4), QColorConstants::Svg::white);
    }

    return checkerPattern;
}

QImage QSvgPattern::patternImage(QPainter *p, QSvgExtraStates &states, const QSvgNode *patternElement)
{
    // pe stands for Pattern Element
    QRectF peBoundingBox;
    QRectF peWorldBoundingBox;

    QTransform t = p->transform();
    p->resetTransform();
    peBoundingBox = patternElement->bounds(p, states);
    peWorldBoundingBox = t.mapRect(peBoundingBox);
    p->setTransform(t);

    // This function renders the pattern into an Image, so we need to apply the correct
    // scaling values when we draw the pattern. The scaling is affected by two factors :
    //      - The "patternTransform" attribute which itself might contain a scaling
    //      - The scaling applied globally.
    // The first is obtained from m11 and m22 matrix elements,
    // while the second is calculated by dividing the patternElement global size
    // by its local size.
    qreal contentScaleFactorX = m_transform.m11();
    qreal contentScaleFactorY = m_transform.m22();
    if (m_contentUnits == QtSvg::UnitTypes::userSpaceOnUse) {
        contentScaleFactorX *= t.m11();
        contentScaleFactorY *= t.m22();
    } else {
        contentScaleFactorX *= peWorldBoundingBox.width();
        contentScaleFactorY *= peWorldBoundingBox.height();
    }

    // Calculate the pattern bounding box depending on the used UnitTypes
    QRectF patternBoundingBox = m_rect.combinedWithLocalRect(peBoundingBox);

    QSize imageSize;
    imageSize.setWidth(qCeil(patternBoundingBox.width() * t.m11() * m_transform.m11()));
    imageSize.setHeight(qCeil(patternBoundingBox.height() * t.m22() * m_transform.m22()));

    calculateAppliedTransform(t, peBoundingBox, imageSize);
    return renderPattern(imageSize, contentScaleFactorX, contentScaleFactorY);
}

QSvgNode::Type QSvgPattern::type() const
{
    return Pattern;
}

QImage QSvgPattern::renderPattern(QSize size, qreal contentScaleX, qreal contentScaleY)
{
    if (size.isEmpty() || !qIsFinite(contentScaleX) || !qIsFinite(contentScaleY))
        return defaultPattern();

    // Allocate a QImage to draw the pattern in with the calculated size.
    QImage pattern;
    if (!QImageIOHandler::allocateImage(size, QImage::Format_ARGB32, &pattern)) {
        qCWarning(lcSvgDraw) << "The requested pattern size is too big, ignoring";
        return defaultPattern();
    }
    pattern.fill(Qt::transparent);

    // Draw the pattern using our QPainter.
    QPainter patternPainter(&pattern);
    QSvgExtraStates patternStates;
    initPainter(&patternPainter);
    applyStyleRecursive(&patternPainter, patternStates);
    patternPainter.resetTransform();

    // According to the <pattern> definition, if viewBox exists then patternContentUnits
    // is ignored
    if (m_viewBox.isNull())
        patternPainter.scale(contentScaleX, contentScaleY);
    else
        patternPainter.setWindow(m_viewBox.toRect());

    // Draw all this Pattern children nodes with our QPainter,
    // no need to use any Extra States
    for (QSvgNode *node : m_renderers)
        node->draw(&patternPainter, patternStates);

    return pattern;
}

void QSvgPattern::calculateAppliedTransform(QTransform &worldTransform, QRectF peLocalBB, QSize imageSize)
{
    // Calculate the required transform to be applied to the QBrush used for correct
    // pattern drawing with the object being rendered.
    // Scale : Apply inverse the scale used above because QBrush uses the transform used
    //         by the QPainter and this function has already rendered the QImage with the
    //         correct size. Moreover, take into account the difference between the required
    //         ideal image size in float and the QSize given to image as an integer value.
    //
    // Translate : Apply translation depending on the calculated x and y values so that the
    //             drawn pattern can be shifted inside the object.
    // Pattern Transform : Apply the transform in the "patternTransform" attribute. This
    //                     transform contains everything except scaling, because it is
    //                     already applied above on the QImage and the QPainter while
    //                     drawing the pattern tile.
    m_appliedTransform.reset();
    qreal imageDownScaleFactorX = 1 / worldTransform.m11();
    qreal imageDownScaleFactorY = 1 / worldTransform.m22();

    m_appliedTransform.scale(qIsFinite(imageDownScaleFactorX) ? imageDownScaleFactorX : 1.0,
                             qIsFinite(imageDownScaleFactorY) ? imageDownScaleFactorY : 1.0);

    QRectF p = m_rect.combinedWithLocalRect(peLocalBB);
    m_appliedTransform.scale((p.width() * worldTransform.m11() * m_transform.m11()) / imageSize.width(),
                             (p.height() * worldTransform.m22() * m_transform.m22()) / imageSize.height());

    QPointF translation = m_rect.translationRelativeToBoundingBox(peLocalBB);
    m_appliedTransform.translate(translation.x() * worldTransform.m11(), translation.y() * worldTransform.m22());

    QTransform scalelessTransform = m_transform;
    scalelessTransform.scale(1 / m_transform.m11(), 1 / m_transform.m22());

    m_appliedTransform = m_appliedTransform * scalelessTransform;
}

QT_END_NAMESPACE
