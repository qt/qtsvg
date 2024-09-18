// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgstyle_p.h"

#include "qsvgfont_p.h"
#include "qsvggraphics_p.h"
#include "qsvgnode_p.h"
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

QSvgStyle::~QSvgStyle()
{
}

void QSvgStyle::apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
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

    for (auto it = animateColors.constBegin(); it != animateColors.constEnd(); ++it)
        (*it)->apply(p, node, states);

    //animated transforms have to be applied
    //_after_ the original object transformations
    if (!animateTransforms.isEmpty()) {
        qreal totalTimeElapsed = node->document()->currentElapsed();
        // Find the last animateTransform with additive="replace", since this will override all
        // previous animateTransforms.
        QList<QSvgRefCounter<QSvgAnimateTransform> >::const_iterator itr = animateTransforms.constEnd();
        do {
            --itr;
            if ((*itr)->animActive(totalTimeElapsed)
                && (*itr)->additiveType() == QSvgAnimateTransform::Replace) {
                // An animateTransform with additive="replace" will replace the transform attribute.
                if (transform)
                    transform->revert(p, states);
                break;
            }
        } while (itr != animateTransforms.constBegin());

        // Apply the animateTransforms after and including the last one with additive="replace".
        for (; itr != animateTransforms.constEnd(); ++itr) {
            if ((*itr)->animActive(totalTimeElapsed))
                (*itr)->apply(p, node, states);
        }
    }

    if (opacity) {
        opacity->apply(p, node, states);
    }

    if (compop) {
        compop->apply(p, node, states);
    }
}

void QSvgStyle::revert(QPainter *p, QSvgExtraStates &states)
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

    //animated transforms need to be reverted _before_
    //the native transforms
    if (!animateTransforms.isEmpty()) {
        QList<QSvgRefCounter<QSvgAnimateTransform> >::const_iterator itr = animateTransforms.constBegin();
        for (; itr != animateTransforms.constEnd(); ++itr) {
            if ((*itr)->transformApplied()) {
                (*itr)->revert(p, states);
                break;
            }
        }
        for (; itr != animateTransforms.constEnd(); ++itr)
            (*itr)->clearTransformApplied();
    }

    if (transform) {
        transform->revert(p, states);
    }

    for (auto it = animateColors.constBegin(); it != animateColors.constEnd(); ++it)
        (*it)->revert(p, states);

    if (opacity) {
        opacity->revert(p, states);
    }

    if (compop) {
        compop->revert(p, states);
    }
}

QSvgAnimate::QSvgAnimate()
    : QSvgStyleProperty(),
      m_from(-1),
      m_totalRunningTime(0),
      m_end(0),
      m_repeatCount(-1.0),
      m_finished(false)

{}

void QSvgAnimate::setRepeatCount(qreal repeatCount)
{
    m_repeatCount = repeatCount;
}

void QSvgAnimate::setRunningTime(int startMs, int durMs, int endMs, int by)
{
    Q_UNUSED(by)
    m_from = startMs;
    m_end = endMs;
    m_totalRunningTime = startMs + durMs;
}

qreal QSvgAnimate::lerp(qreal a, qreal b, qreal t) const
{
    return a + (b - a) * t;
}

qreal QSvgAnimate::currentIterTimeFraction(qreal elpasedTime)
{
    qreal fractionOfTotalTime = 0;
    if (m_totalRunningTime != 0) {
        fractionOfTotalTime = (elpasedTime - m_from) / m_totalRunningTime;
        if (m_repeatCount >= 0 && m_repeatCount < fractionOfTotalTime) {
            m_finished = true;
            fractionOfTotalTime = m_repeatCount;
        }
    }

    return fractionOfTotalTime - std::trunc(fractionOfTotalTime);
}

QSvgAnimateTransform::QSvgAnimateTransform()
    : m_type(Empty),
      m_additive(Replace),
      m_count(0),
      m_freeze(false),
      m_transformApplied(false)
{
}

void QSvgAnimateTransform::setArgs(TransformType type, Additive additive, const QList<qreal> &args)
{
    m_type = type;
    m_args = args;
    m_additive = additive;
    Q_ASSERT(!(args.size()%3));
    m_count = args.size() / 3;
}

void QSvgAnimateTransform::apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &)
{
    m_oldWorldTransform = p->worldTransform();
    resolveMatrix(node);
    p->setWorldTransform(m_transform, true);
    m_transformApplied = true;
}

void QSvgAnimateTransform::revert(QPainter *p, QSvgExtraStates &)
{
    p->setWorldTransform(m_oldWorldTransform, false /* don't combine */);
    m_transformApplied = false;
}

void QSvgAnimateTransform::resolveMatrix(const QSvgNode *node)
{
    qreal elapsedTime = node->document()->currentElapsed();
    if (elapsedTime < m_from || m_finished)
        return;

    qreal fractionOfCurrentIterationTime = currentIterTimeFraction(elapsedTime);
    qreal currentIndexPosition = fractionOfCurrentIterationTime * (m_count - 1);
    int endElem = qCeil(currentIndexPosition);
    int startElem = qMax(endElem - 1, 0);

    qreal fractionOfCurrentElement = currentIndexPosition - std::trunc(currentIndexPosition);

    switch(m_type)
    {
    case Translate: {
        startElem *= 3;
        endElem   *= 3;
        qreal from1, from2;
        qreal to1, to2;
        from1 = m_args[startElem++];
        from2 = m_args[startElem++];
        to1   = m_args[endElem++];
        to2   = m_args[endElem++];

        qreal transX = lerp(from1, to1, fractionOfCurrentElement);
        qreal transY = lerp(from2, to2, fractionOfCurrentElement);
        m_transform = QTransform();
        m_transform.translate(transX, transY);
        break;
    }
    case Scale: {
        startElem *= 3;
        endElem   *= 3;
        qreal from1, from2;
        qreal to1, to2;
        from1 = m_args[startElem++];
        from2 = m_args[startElem++];
        to1   = m_args[endElem++];
        to2   = m_args[endElem++];

        qreal scaleX = lerp(from1, to1, fractionOfCurrentElement);
        qreal scaleY = lerp(from2, to2, fractionOfCurrentElement);
        if (scaleY == 0)
            scaleY = scaleX;
        m_transform = QTransform();
        m_transform.scale(scaleX, scaleY);
        break;
    }
    case Rotate: {
        startElem *= 3;
        endElem   *= 3;
        qreal from1, from2, from3;
        qreal to1, to2, to3;
        from1 = m_args[startElem++];
        from2 = m_args[startElem++];
        from3 = m_args[startElem++];
        to1   = m_args[endElem++];
        to2   = m_args[endElem++];
        to3   = m_args[endElem++];

        qreal rotationDiff = (to1 - from1) * fractionOfCurrentElement;

        qreal transX = lerp(from2, to2, fractionOfCurrentElement);
        qreal transY = lerp(from3, to3, fractionOfCurrentElement);
        m_transform = QTransform();
        m_transform.translate(transX, transY);
        m_transform.rotate(rotationDiff);
        m_transform.translate(-transX, -transY);
        break;
    }
    case SkewX: {
        startElem *= 3;
        endElem   *= 3;
        qreal from1;
        qreal to1;
        from1 = m_args[startElem++];
        to1   = m_args[endElem++];

        qreal skewX = lerp(from1, to1, fractionOfCurrentElement);
        m_transform = QTransform();
        m_transform.shear(qTan(qDegreesToRadians(skewX)), 0);
        break;
    }
    case SkewY: {
        startElem *= 3;
        endElem   *= 3;
        qreal from1;
        qreal to1;
        from1 = m_args[startElem++];
        to1   = m_args[endElem++];

        qreal skewY = lerp(from1, to1, fractionOfCurrentElement);
        m_transform = QTransform();
        m_transform.shear(0, qTan(qDegreesToRadians(skewY)));
        break;
    }
    default:
        break;
    }
}

QSvgStyleProperty::Type QSvgAnimateTransform::type() const
{
    return ANIMATE_TRANSFORM;
}

void QSvgAnimateTransform::setFreeze(bool freeze)
{
    m_freeze = freeze;
}

QSvgAnimateColor::QSvgAnimateColor()
    : m_fill(false),
      m_freeze(false)
{
}

void QSvgAnimateColor::setArgs(bool fill,
                               const QList<QColor> &colors)
{
    m_fill = fill;
    m_colors = colors;
}

void QSvgAnimateColor::setFreeze(bool freeze)
{
    m_freeze = freeze;
}

void QSvgAnimateColor::apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &)
{
    qreal elapsedTime = node->document()->currentElapsed();
    if (elapsedTime < m_from || m_finished)
        return;

    qreal fractionOfCurrentIterationTime = currentIterTimeFraction(elapsedTime);

    qreal currentPosition = fractionOfCurrentIterationTime * (m_colors.size() - 1);

    int startElem = qFloor(currentPosition);
    int endElem   = qCeil(currentPosition);
    QColor start = m_colors[startElem];
    QColor end = m_colors[endElem];

    qreal percentOfColorMorph = currentPosition;
    if (percentOfColorMorph > 1) {
        percentOfColorMorph -= ((int)percentOfColorMorph);
    }

    int alpha  = lerp(start.alpha(), end.alpha(), percentOfColorMorph);
    int red    = lerp(start.red(), end.red(), percentOfColorMorph);
    int green  = lerp(start.green(), end.green(), percentOfColorMorph);
    int blue   = lerp(start.blue(), end.blue(), percentOfColorMorph);;

    QColor color(red, green, blue, alpha);

    if (m_fill) {
        QBrush b = p->brush();
        m_oldBrush = b;
        b.setColor(color);
        p->setBrush(b);
    } else {
        QPen pen = p->pen();
        m_oldPen = pen;
        pen.setColor(color);
        p->setPen(pen);
    }
}

void QSvgAnimateColor::revert(QPainter *p, QSvgExtraStates &)
{
    if (m_fill) {
        p->setBrush(m_oldBrush);
    } else {
        p->setPen(m_oldPen);
    }
}

QSvgStyleProperty::Type QSvgAnimateColor::type() const
{
    return ANIMATE_COLOR;
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

QT_END_NAMESPACE
