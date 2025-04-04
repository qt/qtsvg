// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgstyle_p.h"

#include "qsvgfont_p.h"
#include "qsvggraphics_p.h"
#include "qsvgnode_p.h"
#include "private/qsvganimate_p.h"
#include "qsvgtinydocument_p.h"

#include "qpainter.h"
#include "qpair.h"
#include "qcolor.h"
#include "qdebug.h"
#include "qmath.h"
#include "qnumeric.h"

QT_BEGIN_NAMESPACE

QSvgExtraStates::QSvgExtraStates()
    : fillOpacity(1.0),
      strokeOpacity(1.0),
      svgFont(0),
      textAnchor(Qt::AlignLeft),
      fontWeight(QFont::Normal),
      fillRule(Qt::WindingFill),
      strokeDashOffset(0),
      vectorEffect(false),
      imageRendering(QSvgQualityStyle::ImageRenderingAuto)
{
}

QSvgStyleProperty::~QSvgStyleProperty()
{
}

void QSvgPaintStyleProperty::apply(QPainter *, const QSvgNode *, QSvgExtraStates &)
{
    Q_ASSERT(!"This should not be called!");
}

void QSvgPaintStyleProperty::revert(QPainter *, QSvgExtraStates &)
{
    Q_ASSERT(!"This should not be called!");
}


QSvgQualityStyle::QSvgQualityStyle(int color)
    : m_imageRendering(QSvgQualityStyle::ImageRenderingAuto)
    , m_oldImageRendering(QSvgQualityStyle::ImageRenderingAuto)
    , m_imageRenderingSet(0)
{
    Q_UNUSED(color);
}

void QSvgQualityStyle::setImageRendering(ImageRendering hint) {
    m_imageRendering = hint;
    m_imageRenderingSet = 1;
}

void QSvgQualityStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &states)
{
   m_oldImageRendering = states.imageRendering;
   if (m_imageRenderingSet) {
       states.imageRendering = m_imageRendering;
   }
   if (m_imageRenderingSet) {
       bool smooth = false;
       if (m_imageRendering == ImageRenderingAuto)
           // auto (the spec says to prefer quality)
           smooth = true;
       else
           smooth = (m_imageRendering == ImageRenderingOptimizeQuality);
       p->setRenderHint(QPainter::SmoothPixmapTransform, smooth);
   }
}

void QSvgQualityStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    if (m_imageRenderingSet) {
        states.imageRendering = m_oldImageRendering;
        bool smooth = false;
        if (m_oldImageRendering == ImageRenderingAuto)
            smooth = true;
        else
            smooth = (m_oldImageRendering == ImageRenderingOptimizeQuality);
        p->setRenderHint(QPainter::SmoothPixmapTransform, smooth);
    }
}

QSvgFillStyle::QSvgFillStyle()
    : m_style(0)
    , m_fillRule(Qt::WindingFill)
    , m_oldFillRule(Qt::WindingFill)
    , m_fillOpacity(1.0)
    , m_oldFillOpacity(0)
    , m_paintStyleResolved(1)
    , m_fillRuleSet(0)
    , m_fillOpacitySet(0)
    , m_fillSet(0)
{
}

void QSvgFillStyle::setFillRule(Qt::FillRule f)
{
    m_fillRuleSet = 1;
    m_fillRule = f;
}

void QSvgFillStyle::setFillOpacity(qreal opacity)
{
    m_fillOpacitySet = 1;
    m_fillOpacity = opacity;
}

void QSvgFillStyle::setFillStyle(QSvgPaintStyleProperty* style)
{
    m_style = style;
    m_fillSet = 1;
}

void QSvgFillStyle::setBrush(QBrush brush)
{
    m_fill = std::move(brush);
    m_style = nullptr;
    m_fillSet = 1;
}

void QSvgFillStyle::apply(QPainter *p, const QSvgNode *n, QSvgExtraStates &states)
{
    m_oldFill = p->brush();
    m_oldFillRule = states.fillRule;
    m_oldFillOpacity = states.fillOpacity;

    if (m_fillRuleSet)
        states.fillRule = m_fillRule;
    if (m_fillSet) {
        if (m_style)
            p->setBrush(m_style->brush(p, n, states));
        else
            p->setBrush(m_fill);
    }
    if (m_fillOpacitySet)
        states.fillOpacity = m_fillOpacity;
}

void QSvgFillStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    if (m_fillOpacitySet)
        states.fillOpacity = m_oldFillOpacity;
    if (m_fillSet)
        p->setBrush(m_oldFill);
    if (m_fillRuleSet)
        states.fillRule = m_oldFillRule;
}

QSvgViewportFillStyle::QSvgViewportFillStyle(const QBrush &brush)
    : m_viewportFill(brush)
{
}

void QSvgViewportFillStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &)
{
    m_oldFill = p->brush();
    p->setBrush(m_viewportFill);
}

void QSvgViewportFillStyle::revert(QPainter *p, QSvgExtraStates &)
{
    p->setBrush(m_oldFill);
}

QSvgFontStyle::QSvgFontStyle(QSvgFont *font, QSvgTinyDocument *doc)
    : m_svgFont(font)
    , m_doc(doc)
    , m_familySet(0)
    , m_sizeSet(0)
    , m_styleSet(0)
    , m_variantSet(0)
    , m_weightSet(0)
    , m_textAnchorSet(0)
{
}

QSvgFontStyle::QSvgFontStyle()
    : m_svgFont(0)
    , m_doc(0)
    , m_familySet(0)
    , m_sizeSet(0)
    , m_styleSet(0)
    , m_variantSet(0)
    , m_weightSet(0)
    , m_textAnchorSet(0)
{
}

void QSvgFontStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &states)
{
    m_oldQFont = p->font();
    m_oldSvgFont = states.svgFont;
    m_oldTextAnchor = states.textAnchor;
    m_oldWeight = states.fontWeight;

    if (m_textAnchorSet)
        states.textAnchor = m_textAnchor;

    QFont font = m_oldQFont;
    if (m_familySet) {
        states.svgFont = m_svgFont;
        font.setFamilies(m_qfont.families());
    }

    if (m_sizeSet)
        font.setPointSizeF(m_qfont.pointSizeF());

    if (m_styleSet)
        font.setStyle(m_qfont.style());

    if (m_variantSet)
        font.setCapitalization(m_qfont.capitalization());

    if (m_weightSet) {
        if (m_weight == BOLDER) {
            states.fontWeight = qMin(states.fontWeight + 100, static_cast<int>(QFont::Black));
        } else if (m_weight == LIGHTER) {
            states.fontWeight = qMax(states.fontWeight - 100, static_cast<int>(QFont::Thin));
        } else {
            states.fontWeight = m_weight;
        }
        font.setWeight(QFont::Weight(qBound(static_cast<int>(QFont::Weight::Thin),
                                            states.fontWeight,
                                            static_cast<int>(QFont::Weight::Black))));
    }

    p->setFont(font);
}

void QSvgFontStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    p->setFont(m_oldQFont);
    states.svgFont = m_oldSvgFont;
    states.textAnchor = m_oldTextAnchor;
    states.fontWeight = m_oldWeight;
}

QSvgStrokeStyle::QSvgStrokeStyle()
    : m_strokeOpacity(1.0)
    , m_oldStrokeOpacity(0.0)
    , m_strokeDashOffset(0)
    , m_oldStrokeDashOffset(0)
    , m_style(0)
    , m_paintStyleResolved(1)
    , m_vectorEffect(0)
    , m_oldVectorEffect(0)
    , m_strokeSet(0)
    , m_strokeDashArraySet(0)
    , m_strokeDashOffsetSet(0)
    , m_strokeLineCapSet(0)
    , m_strokeLineJoinSet(0)
    , m_strokeMiterLimitSet(0)
    , m_strokeOpacitySet(0)
    , m_strokeWidthSet(0)
    , m_vectorEffectSet(0)
{
}

void QSvgStrokeStyle::apply(QPainter *p, const QSvgNode *n, QSvgExtraStates &states)
{
    m_oldStroke = p->pen();
    m_oldStrokeOpacity = states.strokeOpacity;
    m_oldStrokeDashOffset = states.strokeDashOffset;
    m_oldVectorEffect = states.vectorEffect;

    QPen pen = p->pen();

    qreal oldWidth = pen.widthF();
    qreal width = m_stroke.widthF();
    if (oldWidth == 0)
        oldWidth = 1;
    if (width == 0)
        width = 1;
    qreal scale = oldWidth / width;

    if (m_strokeOpacitySet)
        states.strokeOpacity = m_strokeOpacity;

    if (m_vectorEffectSet)
        states.vectorEffect = m_vectorEffect;

    if (m_strokeSet) {
        if (m_style)
            pen.setBrush(m_style->brush(p, n, states));
        else
            pen.setBrush(m_stroke.brush());
    }

    if (m_strokeWidthSet)
        pen.setWidthF(m_stroke.widthF());

    bool setDashOffsetNeeded = false;

    if (m_strokeDashOffsetSet) {
        states.strokeDashOffset = m_strokeDashOffset;
        setDashOffsetNeeded = true;
    }

    if (m_strokeDashArraySet) {
        if (m_stroke.style() == Qt::SolidLine) {
            pen.setStyle(Qt::SolidLine);
        } else if (m_strokeWidthSet || oldWidth == 1) {
            // If both width and dash array was set, the dash array is already scaled correctly.
            pen.setDashPattern(m_stroke.dashPattern());
            setDashOffsetNeeded = true;
        } else {
            // If dash array was set, but not the width, the dash array has to be scaled with respect to the old width.
            QList<qreal> dashes = m_stroke.dashPattern();
            for (int i = 0; i < dashes.size(); ++i)
                dashes[i] /= oldWidth;
            pen.setDashPattern(dashes);
            setDashOffsetNeeded = true;
        }
    } else if (m_strokeWidthSet && pen.style() != Qt::SolidLine && scale != 1) {
        // If the width was set, but not the dash array, the old dash array must be scaled with respect to the new width.
        QList<qreal> dashes = pen.dashPattern();
        for (int i = 0; i < dashes.size(); ++i)
            dashes[i] *= scale;
        pen.setDashPattern(dashes);
        setDashOffsetNeeded = true;
    }

    if (m_strokeLineCapSet)
        pen.setCapStyle(m_stroke.capStyle());
    if (m_strokeLineJoinSet)
        pen.setJoinStyle(m_stroke.joinStyle());
    if (m_strokeMiterLimitSet)
        pen.setMiterLimit(m_stroke.miterLimit());

    // You can have dash offset on solid strokes in SVG files, but not in Qt.
    // QPen::setDashOffset() will set the pen style to Qt::CustomDashLine,
    // so don't call the method if the pen is solid.
    if (setDashOffsetNeeded && pen.style() != Qt::SolidLine) {
        qreal currentWidth = pen.widthF();
        if (currentWidth == 0)
            currentWidth = 1;
        pen.setDashOffset(states.strokeDashOffset / currentWidth);
    }

    pen.setCosmetic(states.vectorEffect);

    p->setPen(pen);
}

void QSvgStrokeStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    p->setPen(m_oldStroke);
    states.strokeOpacity = m_oldStrokeOpacity;
    states.strokeDashOffset = m_oldStrokeDashOffset;
    states.vectorEffect = m_oldVectorEffect;
}

void QSvgStrokeStyle::setDashArray(const QList<qreal> &dashes)
{
    if (m_strokeWidthSet) {
        QList<qreal> d = dashes;
        qreal w = m_stroke.widthF();
        if (w != 0 && w != 1) {
            for (int i = 0; i < d.size(); ++i)
                d[i] /= w;
        }
        m_stroke.setDashPattern(d);
    } else {
        m_stroke.setDashPattern(dashes);
    }
    m_strokeDashArraySet = 1;
}

QSvgSolidColorStyle::QSvgSolidColorStyle(const QColor &color)
    : m_solidColor(color)
{
}

QSvgGradientStyle::QSvgGradientStyle(QGradient *grad)
    : m_gradient(grad), m_gradientStopsSet(false)
{
}

QBrush QSvgGradientStyle::brush(QPainter *, const QSvgNode *, QSvgExtraStates &)
{
    if (!m_link.isEmpty()) {
        resolveStops();
    }

    // If the gradient is marked as empty, insert transparent black
    if (!m_gradientStopsSet) {
        m_gradient->setStops(QGradientStops() << QGradientStop(0.0, QColor(0, 0, 0, 0)));
        m_gradientStopsSet = true;
    }

    QBrush b(*m_gradient);

    if (!m_transform.isIdentity())
        b.setTransform(m_transform);

    return b;
}


void QSvgGradientStyle::setTransform(const QTransform &transform)
{
    m_transform = transform;
}

QSvgPatternStyle::QSvgPatternStyle(QSvgPattern *pattern)
    : m_pattern(pattern)
{

}

QBrush QSvgPatternStyle::brush(QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
{
    m_patternImage = m_pattern->patternImage(p, states, node);
    QBrush b(m_patternImage);
    b.setTransform(m_pattern->appliedTransform());
    return b;
}

QSvgTransformStyle::QSvgTransformStyle(const QTransform &trans)
    : m_transform(trans)
{
}

void QSvgTransformStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &)
{
    m_oldWorldTransform.push(p->worldTransform());
    p->setWorldTransform(m_transform, true);
}

void QSvgTransformStyle::revert(QPainter *p, QSvgExtraStates &)
{
    p->setWorldTransform(m_oldWorldTransform.pop(), false /* don't combine */);
}

QSvgStyleProperty::Type QSvgQualityStyle::type() const
{
    return QUALITY;
}

QSvgStyleProperty::Type QSvgFillStyle::type() const
{
    return FILL;
}

QSvgStyleProperty::Type QSvgViewportFillStyle::type() const
{
    return VIEWPORT_FILL;
}

QSvgStyleProperty::Type QSvgFontStyle::type() const
{
    return FONT;
}

QSvgStyleProperty::Type QSvgStrokeStyle::type() const
{
    return STROKE;
}

QSvgStyleProperty::Type QSvgSolidColorStyle::type() const
{
    return SOLID_COLOR;
}

QSvgStyleProperty::Type QSvgGradientStyle::type() const
{
    return GRADIENT;
}

QSvgStyleProperty::Type QSvgPatternStyle::type() const
{
    return PATTERN;
}

QSvgStyleProperty::Type QSvgTransformStyle::type() const
{
    return TRANSFORM;
}


QSvgCompOpStyle::QSvgCompOpStyle(QPainter::CompositionMode mode)
    : m_mode(mode)
{

}

void QSvgCompOpStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &)
{
    m_oldMode = p->compositionMode();
    p->setCompositionMode(m_mode);
}

void QSvgCompOpStyle::revert(QPainter *p, QSvgExtraStates &)
{
    p->setCompositionMode(m_oldMode);
}

QSvgStyleProperty::Type QSvgCompOpStyle::type() const
{
    return COMP_OP;
}

QSvgOpacityStyle::QSvgOpacityStyle(qreal opacity)
    : m_opacity(opacity), m_oldOpacity(0)
{

}

void QSvgOpacityStyle::apply(QPainter *p, const QSvgNode *, QSvgExtraStates &)
{
    m_oldOpacity = p->opacity();
    p->setOpacity(m_opacity * m_oldOpacity);
}

void QSvgOpacityStyle::revert(QPainter *p, QSvgExtraStates &)
{
    p->setOpacity(m_oldOpacity);
}

QSvgStyleProperty::Type QSvgOpacityStyle::type() const
{
    return OPACITY;
}

void QSvgGradientStyle::setStopLink(const QString &link, QSvgTinyDocument *doc)
{
    m_link = link;
    m_doc  = doc;
}

void QSvgGradientStyle::resolveStops()
{
    QStringList visited;
    resolveStops_helper(&visited);
}

void QSvgGradientStyle::resolveStops_helper(QStringList *visited)
{
    if (!m_link.isEmpty() && m_doc) {
        QSvgStyleProperty *prop = m_doc->styleProperty(m_link);
        if (prop && !visited->contains(m_link)) {
            visited->append(m_link);
            if (prop->type() == QSvgStyleProperty::GRADIENT) {
                QSvgGradientStyle *st =
                    static_cast<QSvgGradientStyle*>(prop);
                st->resolveStops_helper(visited);
                m_gradient->setStops(st->qgradient()->stops());
                m_gradientStopsSet = st->gradientStopsSet();
            }
        } else {
            qWarning("Could not resolve property : %s", qPrintable(m_link));
        }
        m_link = QString();
    }
}

QSvgStaticStyle::QSvgStaticStyle()
    : quality(0)
    , fill(0)
    , viewportFill(0)
    , font(0)
    , stroke(0)
    , solidColor(0)
    , gradient(0)
    , pattern(0)
    , transform(0)
    , opacity(0)
    , compop(0)
{
}

QSvgStaticStyle::~QSvgStaticStyle()
{
}

void QSvgStaticStyle::apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
{
    if (quality) {
        quality->apply(p, node, states);
    }

    if (fill) {
        fill->apply(p, node, states);
    }

    if (viewportFill) {
        viewportFill->apply(p, node, states);
    }

    if (font) {
        font->apply(p, node, states);
    }

    if (stroke) {
        stroke->apply(p, node, states);
    }

    if (transform) {
        transform->apply(p, node, states);
    }

    if (opacity) {
        opacity->apply(p, node, states);
    }

    if (compop) {
        compop->apply(p, node, states);
    }
}

void QSvgStaticStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    if (quality) {
        quality->revert(p, states);
    }

    if (fill) {
        fill->revert(p, states);
    }

    if (viewportFill) {
        viewportFill->revert(p, states);
    }

    if (font) {
        font->revert(p, states);
    }

    if (stroke) {
        stroke->revert(p, states);
    }

    if (transform) {
        transform->revert(p, states);
    }

    if (opacity) {
        opacity->revert(p, states);
    }

    if (compop) {
        compop->revert(p, states);
    }
}

namespace {

QColor sumValue(const QColor &c1, const QColor &c2)
{
    QRgb rgb1 = c1.rgba();
    QRgb rgb2 = c2.rgba();
    int sumRed = qRed(rgb1) + qRed(rgb2);
    int sumGreen = qGreen(rgb1) + qGreen(rgb2);
    int sumBlue = qBlue(rgb1) + qBlue(rgb2);

    QRgb sumRgb = qRgba(qBound(0, sumRed, 255),
                        qBound(0, sumGreen, 255),
                        qBound(0, sumBlue, 255),
                        255);

    return QColor(sumRgb);
}

qreal sumValue(qreal value1, qreal value2)
{
    qreal sumValue = value1 + value2;
    return qBound(0.0, sumValue, 1.0);
}

}

QSvgAnimatedStyle::QSvgAnimatedStyle()
{
}

QSvgAnimatedStyle::~QSvgAnimatedStyle()
{
}

void QSvgAnimatedStyle::apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
{
    QSharedPointer<QSvgAbstractAnimator> animator = node->document()->animator();
    QList<QSvgAbstractAnimation *> nodeAnims = animator->animationsForNode(node);

    savePaintingState(p, node, states);
    if (nodeAnims.isEmpty())
        return;

    for (auto anim : nodeAnims) {
        if (!anim->isActive())
            continue;

        bool replace = anim->animationType() == QSvgAbstractAnimation::CSS ? true :
                           (static_cast<QSvgAnimateNode *>(anim))->additiveType() == QSvgAnimateNode::Replace;
        QList<QSvgAbstractAnimatedProperty *> props = anim->properties();
        for (auto prop : props)
            applyPropertyAnimation(p, prop, replace, states);
    }
}

void QSvgAnimatedStyle::revert(QPainter *p, QSvgExtraStates &states)
{
    p->setWorldTransform(m_worldTransform, false);
    p->setBrush(m_brush);
    p->setPen(m_pen);
    p->setOpacity(m_opacity);
    states.fillOpacity = m_fillOpacity;
    states.strokeOpacity = m_strokeOpacity;
}

void QSvgAnimatedStyle::savePaintingState(const QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
{
    QSvgStaticStyle style = node->style();
    m_worldTransform = m_transformToNode = p->worldTransform();
    if (style.transform)
        m_transformToNode = style.transform->qtransform().inverted() * m_transformToNode;

    m_brush = p->brush();
    m_pen = p->pen();
    m_fillOpacity = states.fillOpacity;
    m_strokeOpacity = states.strokeOpacity;
    m_opacity = p->opacity();
}

void QSvgAnimatedStyle::applyPropertyAnimation(QPainter *p, QSvgAbstractAnimatedProperty *property,
                                               bool replace, QSvgExtraStates &states)
{
    if (property->propertyName() == QStringLiteral("fill")) {
        QBrush brush = p->brush();
        QColor brushColor = brush.color();
        QColor animatedColor = property->interpolatedValue().value<QColor>();
        QColor sumOrReplaceColor = replace ? animatedColor : sumValue(brushColor, animatedColor);
        brush.setColor(sumOrReplaceColor);
        p->setBrush(brush);
    } else if (property->propertyName() == QStringLiteral("stroke")) {
        QPen pen = p->pen();
        QBrush penBrush = pen.brush();
        QColor penColor = penBrush.color();
        QColor animatedColor = property->interpolatedValue().value<QColor>();
        QColor sumOrReplaceColor = replace ? animatedColor : sumValue(penColor, animatedColor);
        penBrush.setColor(sumOrReplaceColor);
        penBrush.setStyle(Qt::SolidPattern);
        pen.setBrush(penBrush);
        p->setPen(pen);
    } else if (property->propertyName() == QStringLiteral("transform")) {
        QTransform animatedTransform = property->interpolatedValue().value<QTransform>();
        QTransform sumOrReplaceTransform = replace ? animatedTransform * m_transformToNode :
                                               animatedTransform * p->worldTransform();
        p->setWorldTransform(sumOrReplaceTransform);
    } else if (property->propertyName() == QStringLiteral("fill-opacity")) {
        qreal animatedFillOpacity = property->interpolatedValue().value<qreal>();
        qreal sumOrReplaceOpacity = replace ? animatedFillOpacity : sumValue(m_fillOpacity, animatedFillOpacity);
        states.fillOpacity = sumOrReplaceOpacity;
    } else if (property->propertyName() == QStringLiteral("stroke-opacity")) {
        qreal animatedStrokeOpacity = property->interpolatedValue().value<qreal>();
        qreal sumOrReplaceOpacity = replace ? animatedStrokeOpacity : sumValue(m_strokeOpacity, animatedStrokeOpacity);
        states.strokeOpacity = sumOrReplaceOpacity;
    } else if (property->propertyName() == QStringLiteral("opacity")) {
        qreal animatedOpacity = property->interpolatedValue().value<qreal>();
        qreal sumOrReplaceOpacity = replace ? animatedOpacity : sumValue(m_opacity, animatedOpacity);
        p->setOpacity(sumOrReplaceOpacity);
    }
}

QT_END_NAMESPACE
