// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSVGSTYLE_P_H
#define QSVGSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qstack.h"
#include "QtGui/qpainter.h"
#include "QtGui/qpainterpath.h"
#include "QtGui/qpen.h"
#include "QtGui/qbrush.h"
#include "QtGui/qtransform.h"
#include "QtGui/qfont.h"
#include <qdebug.h>
#include "qtsvgglobal_p.h"
#include <QtSvg/private/qsvgpaintserver_p.h>
#include <memory>
#include <array>

QT_BEGIN_NAMESPACE

class QPainter;
class QSvgNode;
class QSvgFont;
class QSvgDocument;

template <class T> class QSvgRefCounter
{
public:
    QSvgRefCounter() { t = nullptr; }
    explicit QSvgRefCounter(T *_t)
    {
        t = _t;
        if (t)
            t->ref();
    }

    QSvgRefCounter(const QSvgRefCounter &other)
    {
        t = other.t;
        if (t)
            t->ref();
    }

    QSvgRefCounter(QSvgRefCounter &&other) noexcept
        : t{std::exchange(other.t, nullptr)} {}

    QSvgRefCounter &operator =(const QSvgRefCounter &other)
    {
        if(other.t)
            other.t->ref();
        if (t)
            t->deref();
        t = other.t;
        return *this;
    }

    // both T and users are a bounded set, so we can use PURE_SWAP,
    // and manage expectations
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QSvgRefCounter)

    ~QSvgRefCounter()
    {
        if (t)
            t->deref();
    }

    void swap(QSvgRefCounter &other) noexcept
    { qt_ptr_swap(t, other.t); }

    void reset(T *other = nullptr)
    { QSvgRefCounter(other).swap(*this); }

    inline T *operator->() const { return t; }
    inline operator T*() const { return t; }

    inline bool isDefault() const { return !t || t->isDefault(); }

private:
    T *t;
};

class Q_SVG_EXPORT QSvgRefCounted
{
    Q_DISABLE_COPY_MOVE(QSvgRefCounted)
public:
    QSvgRefCounted() { _ref = 0; }
    virtual ~QSvgRefCounted();

    void ref() {
        ++_ref;
//        qDebug() << this << ": adding ref, now " << _ref;
    }
    void deref() {
//        qDebug() << this << ": removing ref, now " << _ref;
        if(!--_ref) {
//            qDebug("     deleting");
            delete this;
        }
    }
private:
    int _ref;
};

struct Q_SVG_EXPORT QSvgExtraStates
{
    QSvgExtraStates();

    qreal fillOpacity;
    qreal strokeOpacity;
    QSvgFont *svgFont;
    Qt::Alignment textAnchor;
    int fontWeight;
    Qt::FillRule fillRule;
    qreal strokeDashOffset;
    int nestedUseLevel = 0;
    int nestedUseCount = 0;
    bool trustedSource = false;
    quint8 remainingNestedNodes = QtSvg::renderingMaxNestedNodes;
    bool vectorEffect; // true if pen is cosmetic
    qint8 imageRendering; // QSvgQualityStyle::ImageRendering
    bool inUse = false; // true if currently in QSvgUseNode
};

class Q_SVG_EXPORT QSvgStyleProperty
{
public:
    enum Type : quint8
    {
        Quality,
        Fill,
        ViewportFill,
        Font,
        Stroke,
        Transform,
        Opacity,
        CompOp,
        Offset,

        NumTypes
    };
public:
    virtual ~QSvgStyleProperty();

    virtual void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) = 0;
    virtual void revert(QPainter *p, QSvgExtraStates &states) = 0;
    virtual Type type() const = 0;
    virtual bool isDefault() const { return false; }
};

class Q_SVG_EXPORT QSvgQualityStyle : public QSvgStyleProperty
{
public:
    enum ImageRendering: qint8 {
        ImageRenderingAuto = 0,
        ImageRenderingOptimizeSpeed = 1,
        ImageRenderingOptimizeQuality = 2,
    };

    QSvgQualityStyle(int color);
    ~QSvgQualityStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setImageRendering(ImageRendering);
private:
    // color-render ing v 	v 	'auto' | 'optimizeSpeed' |
    //                                  'optimizeQuality' | 'inherit'
    //int m_colorRendering;

    // shape-rendering v 	v 	'auto' | 'optimizeSpeed' | 'crispEdges' |
    //                                  'geometricPrecision' | 'inherit'
    //QSvgShapeRendering m_shapeRendering;


    // text-rendering    v 	v 	'auto' | 'optimizeSpeed' | 'optimizeLegibility'
    //                                | 'geometricPrecision' | 'inherit'
    //QSvgTextRendering m_textRendering;


    // vector-effect         v 	x 	'default' | 'non-scaling-stroke' | 'inherit'
    //QSvgVectorEffect m_vectorEffect;

    // image-rendering v 	v 	'auto' | 'optimizeSpeed' | 'optimizeQuality' |
    //                                      'inherit'
    qint32 m_imageRendering: 4;
    qint32 m_oldImageRendering: 4;
    quint32 m_imageRenderingSet: 1;
};



class Q_SVG_EXPORT QSvgOpacityStyle : public QSvgStyleProperty
{
public:
    QSvgOpacityStyle(qreal opacity);
    ~QSvgOpacityStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    qreal opacity() const { return m_opacity; }
    bool isDefault() const override
    {
        return qFuzzyCompare(m_opacity, qreal(1.0));
    }

private:
    qreal m_opacity;
    qreal m_oldOpacity;
};

class Q_SVG_EXPORT QSvgFillStyle : public QSvgStyleProperty
{
public:
    QSvgFillStyle();
    ~QSvgFillStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setFillRule(Qt::FillRule f);
    void setFillOpacity(qreal opacity);
    void setPaintServer(QSvgPaintServerSharedPtr paintServer);
    void setBrush(QBrush brush);

    const QBrush & qbrush() const
    {
        return m_fill;
    }

    qreal fillOpacity() const
    {
        return m_fillOpacity;
    }

    Qt::FillRule fillRule() const
    {
        return m_fillRule;
    }

    QSvgPaintServer *paintServer() const
    {
        return m_paintServer.get();
    }

    void setPaintStyleId(const QString &Id)
    {
        m_paintStyleId = Id;
    }

    QString paintStyleId() const
    {
        return m_paintStyleId;
    }

private:
    // fill            v 	v 	'inherit' | <Paint.datatype>
    // fill-opacity    v 	v 	'inherit' | <OpacityValue.datatype>
    QBrush m_fill;
    QBrush m_oldFill;
    QSvgPaintServerSharedPtr m_paintServer;

    Qt::FillRule m_fillRule{Qt::WindingFill};
    Qt::FillRule m_oldFillRule{Qt::WindingFill};
    qreal m_fillOpacity{1.0};
    qreal m_oldFillOpacity{0.};

    QString m_paintStyleId;

    uint m_fillRuleSet : 1;
    uint m_fillOpacitySet : 1;
    uint m_fillSet : 1;
};

class Q_SVG_EXPORT QSvgViewportFillStyle : public QSvgStyleProperty
{
public:
    QSvgViewportFillStyle(const QBrush &brush);
    ~QSvgViewportFillStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QBrush & qbrush() const
    {
        return m_viewportFill;
    }
private:
    // viewport-fill         v 	x 	'inherit' | <Paint.datatype>
    // viewport-fill-opacity 	v 	x 	'inherit' | <OpacityValue.datatype>
    QBrush m_viewportFill;

    QBrush m_oldFill;
};

class Q_SVG_EXPORT QSvgFontStyle : public QSvgStyleProperty
{
public:
    static const int LIGHTER = -1;
    static const int BOLDER = 1;

    QSvgFontStyle(QSvgFont *font);
    QSvgFontStyle();
    ~QSvgFontStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setSize(qreal size)
    {
        // Store the _pixel_ size in the font. Since QFont::setPixelSize() only takes an int, call
        // QFont::SetPointSize() instead. Set proper font size just before rendering.
        m_qfont.setPointSizeF(size);
        m_sizeSet = 1;
    }

    void setTextAnchor(Qt::Alignment anchor)
    {
        m_textAnchor = anchor;
        m_textAnchorSet = 1;
    }

    void setFamily(const QString &family)
    {
        m_qfont.setFamilies({family});
        m_familySet = 1;
    }

    void setStyle(QFont::Style fontStyle) {
        m_qfont.setStyle(fontStyle);
        m_styleSet = 1;
    }

    void setVariant(QFont::Capitalization fontVariant)
    {
        m_qfont.setCapitalization(fontVariant);
        m_variantSet = 1;
    }

    void setWeight(int weight)
    {
        m_weight = weight;
        m_weightSet = 1;
    }

    QSvgFont * svgFont() const
    {
        return m_svgFont;
    }

    const QFont &qfont() const
    {
        return m_qfont;
    }

private:
    QSvgFont *m_svgFont;
    QFont m_qfont;

    int m_weight = 0;
    Qt::Alignment m_textAnchor;

    QSvgFont *m_oldSvgFont = nullptr;
    QFont m_oldQFont;
    Qt::Alignment m_oldTextAnchor;
    int m_oldWeight = 0;

    uint m_familySet : 1;
    uint m_sizeSet : 1;
    uint m_styleSet : 1;
    uint m_variantSet : 1;
    uint m_weightSet : 1;
    uint m_textAnchorSet : 1;
};

class Q_SVG_EXPORT QSvgStrokeStyle : public QSvgStyleProperty
{
public:
    QSvgStrokeStyle();
    ~QSvgStrokeStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setStroke(QBrush brush)
    {
        m_stroke.setBrush(brush);
        m_paintServer.reset();
        m_strokeSet = 1;
    }

    void setPaintServer(QSvgPaintServerSharedPtr paintServer)
    {
        m_paintServer = std::move(paintServer);
        m_strokeSet = 1;
    }

    void setDashArray(const QList<qreal> &dashes);

    void setDashArrayNone()
    {
        m_stroke.setStyle(Qt::SolidLine);
        m_strokeDashArraySet = 1;
    }

    void setDashOffset(qreal offset)
    {
        m_strokeDashOffset = offset;
        m_strokeDashOffsetSet = 1;
    }

    void setLineCap(Qt::PenCapStyle cap)
    {
        m_stroke.setCapStyle(cap);
        m_strokeLineCapSet = 1;
    }

    void setLineJoin(Qt::PenJoinStyle join)
    {
        m_stroke.setJoinStyle(join);
        m_strokeLineJoinSet = 1;
    }

    void setMiterLimit(qreal limit)
    {
        m_stroke.setMiterLimit(limit);
        m_strokeMiterLimitSet = 1;
    }

    void setOpacity(qreal opacity)
    {
        m_strokeOpacity = opacity;
        m_strokeOpacitySet = 1;
    }

    void setWidth(qreal width)
    {
        m_stroke.setWidthF(width);
        m_strokeWidthSet = 1;
        Q_ASSERT(!m_strokeDashArraySet); // set width before dash array.
    }

    qreal width()
    {
        return m_stroke.widthF();
    }

    void setVectorEffect(bool nonScalingStroke)
    {
        m_vectorEffect = nonScalingStroke;
        m_vectorEffectSet = 1;
    }

    QSvgPaintServer *paintServer() const
    {
        return m_paintServer.get();
    }

    void setPaintStyleId(const QString &Id)
    {
        m_paintStyleId = Id;
    }

    QString paintStyleId() const
    {
        return m_paintStyleId;
    }

    QPen stroke() const
    {
        return m_stroke;
    }

private:
    // stroke            v 	v 	'inherit' | <Paint.datatype>
    // stroke-dasharray  v 	v 	'inherit' | <StrokeDashArrayValue.datatype>
    // stroke-dashoffset v 	v 	'inherit' | <StrokeDashOffsetValue.datatype>
    // stroke-linecap    v 	v 	'butt' | 'round' | 'square' | 'inherit'
    // stroke-linejoin   v 	v 	'miter' | 'round' | 'bevel' | 'inherit'
    // stroke-miterlimit v 	v 	'inherit' | <StrokeMiterLimitValue.datatype>
    // stroke-opacity    v 	v 	'inherit' | <OpacityValue.datatype>
    // stroke-width      v 	v 	'inherit' | <StrokeWidthValue.datatype>
    QPen m_stroke;
    QPen m_oldStroke;
    qreal m_strokeOpacity{1.0};
    qreal m_oldStrokeOpacity{0.};
    qreal m_strokeDashOffset{0.};
    qreal m_oldStrokeDashOffset{0.};

    QSvgPaintServerSharedPtr m_paintServer;
    QString m_paintStyleId;
    uint m_vectorEffect : 1;
    uint m_oldVectorEffect : 1;

    uint m_strokeSet : 1;
    uint m_strokeDashArraySet : 1;
    uint m_strokeDashOffsetSet : 1;
    uint m_strokeLineCapSet : 1;
    uint m_strokeLineJoinSet : 1;
    uint m_strokeMiterLimitSet : 1;
    uint m_strokeOpacitySet : 1;
    uint m_strokeWidthSet : 1;
    uint m_vectorEffectSet : 1;
};

class Q_SVG_EXPORT QSvgTransformStyle : public QSvgStyleProperty
{
public:
    QSvgTransformStyle(const QTransform &transform);
    ~QSvgTransformStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QTransform & qtransform() const
    {
        return m_transform;
    }
    bool isDefault() const override { return m_transform.isIdentity(); }
private:
    //7.6 The transform  attribute
    QTransform m_transform;
    QStack<QTransform> m_oldWorldTransform;
};

class Q_SVG_EXPORT QSvgCompOpStyle : public QSvgStyleProperty
{
public:
    QSvgCompOpStyle(QPainter::CompositionMode mode);
    ~QSvgCompOpStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    const QPainter::CompositionMode & compOp() const
    {
        return m_mode;
    }
private:
    //comp-op attribute
    QPainter::CompositionMode m_mode;

    QPainter::CompositionMode m_oldMode{QPainter::CompositionMode_SourceOver};
};

class Q_SVG_EXPORT QSvgOffsetStyle : public QSvgStyleProperty
{
public:
    QSvgOffsetStyle() = default;
    ~QSvgOffsetStyle() override;

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) override;
    void revert(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;

    void setPath(const QPainterPath &path)
    {
        m_path = path;
    }

    const QPainterPath &path() const
    {
        return m_path;
    }

    void setRotateAngle(qreal angle)
    {
        m_rotateAngle = angle;
    }

    qreal rotateAngle() const
    {
        return m_rotateAngle;
    }

    void setRotateType(QtSvg::OffsetRotateType type)
    {
        m_rotateType = type;
    }

    QtSvg::OffsetRotateType rotateType() const
    {
        return m_rotateType;
    }

    void setDistance(qreal distance)
    {
        m_distance = distance;
    }

    qreal distance() const
    {
        return m_distance;
    }

private:
    QPainterPath m_path;
    qreal m_distance{0.};
    qreal m_rotateAngle{0.};
    QtSvg::OffsetRotateType m_rotateType{QtSvg::OffsetRotateType::Auto};
};

using QSvgStylePropertyPtr = std::unique_ptr<QSvgStyleProperty>;
using QSvgQualityStylePtr = std::unique_ptr<QSvgQualityStyle>;
using QSvgOpacityStylePtr = std::unique_ptr<QSvgOpacityStyle>;
using QSvgFillStylePtr = std::unique_ptr<QSvgFillStyle>;
using QSvgViewportFillStylePtr = std::unique_ptr<QSvgViewportFillStyle>;
using QSvgFontStylePtr = std::unique_ptr<QSvgFontStyle>;
using QSvgStrokeStylePtr = std::unique_ptr<QSvgStrokeStyle>;
using QSvgTransformStylePtr = std::unique_ptr<QSvgTransformStyle>;
using QSvgCompOpStylePtr = std::unique_ptr<QSvgCompOpStyle>;
using QSvgOffsetStylePtr = std::unique_ptr<QSvgOffsetStyle>;

class Q_SVG_EXPORT QSvgStaticStyle
{
public:
    QSvgStaticStyle();
    ~QSvgStaticStyle();

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void revert(QPainter *p, QSvgExtraStates &states);

    void appendProperty(QSvgStylePropertyPtr prop)
    {
        Q_ASSERT(prop->type() < QSvgStyleProperty::NumTypes);
        m_properties[prop->type()] = std::move(prop);
    }

    QSvgStyleProperty *property(QSvgStyleProperty::Type type) const
    {
        Q_ASSERT(type < QSvgStyleProperty::NumTypes);
        return m_properties.at(type).get();
    }

    bool isDefaultProperty(QSvgStyleProperty::Type type) const
    {
        QSvgStyleProperty *prop = property(type);
        return !prop || prop->isDefault();
    }

private:
    std::array<QSvgStylePropertyPtr, QSvgStyleProperty::NumTypes> m_properties;
};

class QSvgAbstractAnimation;

struct QSvgStyleState
{
    QBrush fill;
    QPen stroke;
    qreal fillOpacity;
    qreal strokeOpacity;
    qreal opacity;
    QTransform transform;
    std::optional<QPainterPath> offsetPath;
    qreal offsetDistance;
    qreal offsetRotate;
    QtSvg::OffsetRotateType offsetRotateType;
};

class Q_SVG_EXPORT QSvgAnimatedStyle
{
public:
    QSvgAnimatedStyle();
    ~QSvgAnimatedStyle();

    void apply(QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void revert(QPainter *p, QSvgExtraStates &states);

private:
    void savePaintingState(const QPainter *p, const QSvgNode *node, QSvgExtraStates &states);
    void fetchStyleState(const QSvgAbstractAnimation *animation, QSvgStyleState &currentStyle);
    void applyStyle(QPainter *p, QSvgExtraStates &states, const QSvgStyleState &currentStyle);

private:
    QTransform m_worldTransform;
    QTransform m_transformToNode;
    QSvgStyleState m_static;
};

/********************************************************/
// NOT implemented:

// color           v 	v 	'inherit' | <Color.datatype>
//QColor m_color;

// display         v 	x 	'inline' | 'block' | 'list-item'
//                                 | 'run-in' | 'compact' | 'marker' |
//                                 'table' | 'inline-table' |
//                                 'table-row-group' | 'table-header-group' |
//                                 'table-footer-group' | 'table-row' |
//                                 'table-column-group' | 'table-column' |
//                                 'table-cell' | 'table-caption' |
//                                 'none' | 'inherit'
//QSvgDisplayStyle m_display;

// display-align   v 	v 	'auto' | 'before' | 'center' | 'after' | 'inherit'
//QSvgDisplayAlign m_displayAlign;

// line-increment  v 	v 	'auto' | 'inherit' | <Number.datatype>
//int m_lineIncrement;

// text-anchor       v 	v 	'start' | 'middle' | 'end' | 'inherit'
//QSvgTextAnchor m_textAnchor;

// visibility 	v 	v 	'visible' | 'hidden' | 'inherit'
//QSvgVisibility m_visibility;

/******************************************************/
// the following do not make sense for us

// pointer-events  v 	v 	'visiblePainted' | 'visibleFill' | 'visibleStroke' |
//                              'visible' | 'painted' | 'fill' | 'stroke' | 'all' |
//                              'none' | 'inherit'
//QSvgPointEvents m_pointerEvents;

// audio-level     v  	x  	'inherit' | <Number.datatype>

QT_END_NAMESPACE

#endif // QSVGSTYLE_P_H
