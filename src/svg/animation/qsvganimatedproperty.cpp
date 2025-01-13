// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvganimatedproperty_p.h"
#include <QtCore/qpoint.h>
#include <QtGui/qcolor.h>
#include <QtGui/qtransform.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qglobalstatic.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcSvgAnimatedProperty, "qt.svg.animation.properties")

typedef QHash<QString, QSvgAbstractAnimatedProperty::Type> AnimatableHashType;
Q_GLOBAL_STATIC(AnimatableHashType, animatableProperties)

static void initHash()
{
    animatableProperties->insert(QStringLiteral("fill"), QSvgAbstractAnimatedProperty::Color);
    animatableProperties->insert(QStringLiteral("stroke"), QSvgAbstractAnimatedProperty::Color);
    animatableProperties->insert(QStringLiteral("transform"), QSvgAbstractAnimatedProperty::Transform);
}

static qreal q_lerp(qreal a, qreal b, qreal t)
{
    return a + (b - a) * t;
}

static QPointF pointInterpolator(QPointF v1, QPointF v2, qreal t)
{
    qreal x = q_lerp(v1.x(), v2.x(), t);
    qreal y = q_lerp(v1.y(), v2.y(), t);

    return QPointF(x, y);
}


QSvgAbstractAnimatedProperty::QSvgAbstractAnimatedProperty(const QString &name, Type type)
    : m_propertyName(name)
    , m_type(type)
{
}

QSvgAbstractAnimatedProperty::~QSvgAbstractAnimatedProperty()
{
}

void QSvgAbstractAnimatedProperty::setKeyFrames(const QList<qreal> &keyFrames)
{
    m_keyFrames = keyFrames;
}

void QSvgAbstractAnimatedProperty::appendKeyFrame(qreal keyFrame)
{
    m_keyFrames.append(keyFrame);
}

QList<qreal> QSvgAbstractAnimatedProperty::keyFrames() const
{
    return m_keyFrames;
}

void QSvgAbstractAnimatedProperty::setPropertyName(const QString &name)
{
    m_propertyName = name;
}

QStringView QSvgAbstractAnimatedProperty::propertyName() const
{
    return m_propertyName;
}

QSvgAbstractAnimatedProperty::Type QSvgAbstractAnimatedProperty::type() const
{
    return m_type;
}

QVariant QSvgAbstractAnimatedProperty::interpolatedValue() const
{
    return m_interpolatedValue;
}

QSvgAbstractAnimatedProperty *QSvgAbstractAnimatedProperty::createAnimatedProperty(const QString &name)
{
    if (animatableProperties->isEmpty())
        initHash();

    if (!animatableProperties->contains(name)) {
        qCDebug(lcSvgAnimatedProperty) << "Property : " << name << " is not animatable";
        return nullptr;
    }

    QSvgAbstractAnimatedProperty::Type type = animatableProperties->value(name);
    QSvgAbstractAnimatedProperty *prop = nullptr;

    switch (type) {
    case QSvgAbstractAnimatedProperty::Color:
        prop = new QSvgAnimatedPropertyColor(name);
        break;
    case QSvgAbstractAnimatedProperty::Transform:
        prop = new QSvgAnimatedPropertyTransform(name);
        break;
    default:
        break;
    }

    return prop;
}

QSvgAnimatedPropertyColor::QSvgAnimatedPropertyColor(const QString &name)
    : QSvgAbstractAnimatedProperty(name, QSvgAbstractAnimatedProperty::Color)
{
}

void QSvgAnimatedPropertyColor::setColors(const QList<QColor> &colors)
{
    m_colors = colors;
}

void QSvgAnimatedPropertyColor::appendColor(const QColor &color)
{
    m_colors.append(color);
}

QList<QColor> QSvgAnimatedPropertyColor::colors() const
{
    return m_colors;
}

void QSvgAnimatedPropertyColor::interpolate(uint index, qreal t)
{
    QColor c1 = m_colors.at(index - 1);
    QColor c2 = m_colors.at(index);

    int alpha  = q_lerp(c1.alpha(), c2.alpha(), t);
    int red    = q_lerp(c1.red(), c2.red(), t);
    int green  = q_lerp(c1.green(), c2.green(), t);
    int blue   = q_lerp(c1.blue(), c2.blue(), t);

    m_interpolatedValue = QColor(red, green, blue, alpha);
}

QSvgAnimatedPropertyTransform::QSvgAnimatedPropertyTransform(const QString &name)
    : QSvgAbstractAnimatedProperty(name, QSvgAbstractAnimatedProperty::Transform)
{

}

void QSvgAnimatedPropertyTransform::setTranslations(const QList<QPointF> &translations)
{
    m_translations = translations;
}

void QSvgAnimatedPropertyTransform::appendTranslation(const QPointF &translation)
{
    m_translations.append(translation);
}

QList<QPointF> QSvgAnimatedPropertyTransform::translations() const
{
    return m_translations;
}

void QSvgAnimatedPropertyTransform::setScales(const QList<QPointF> &scales)
{
    m_scales = scales;
}

void QSvgAnimatedPropertyTransform::appendScale(const QPointF &scale)
{
    m_scales.append(scale);
}

QList<QPointF> QSvgAnimatedPropertyTransform::scales() const
{
    return m_scales;
}

void QSvgAnimatedPropertyTransform::setRotations(const QList<qreal> &rotations)
{
    m_rotations = rotations;
}

void QSvgAnimatedPropertyTransform::appendRotation(qreal rotation)
{
    m_rotations.append(rotation);
}

QList<qreal> QSvgAnimatedPropertyTransform::rotations() const
{
    return m_rotations;
}

void QSvgAnimatedPropertyTransform::setCentersOfRotation(const QList<QPointF> &centersOfRotations)
{
    m_centersOfRotation = centersOfRotations;
}

void QSvgAnimatedPropertyTransform::appendCenterOfRotation(const QPointF &centerOfRotation)
{
    m_centersOfRotation.append(centerOfRotation);
}

QList<QPointF> QSvgAnimatedPropertyTransform::centersOfRotations() const
{
    return m_centersOfRotation;
}

void QSvgAnimatedPropertyTransform::setSkews(const QList<QPointF> &skews)
{
    m_skews = skews;
}

void QSvgAnimatedPropertyTransform::appendSkew(const QPointF &skew)
{
    m_skews.append(skew);
}

QList<QPointF> QSvgAnimatedPropertyTransform::skews() const
{
    return m_skews;
}

qreal QSvgAnimatedPropertyTransform::interpolatedRotation(uint index, qreal t) const
{
    qreal r1 = m_rotations.at(index - 1);
    qreal r2 = m_rotations.at(index);
    return q_lerp(r1, r2, t);
}

QPointF QSvgAnimatedPropertyTransform::interpolatedCenterOfRotation(uint index, qreal t) const
{
    QPointF cor1 = m_centersOfRotation.at(index - 1);
    QPointF cor2 = m_centersOfRotation.at(index);
    return pointInterpolator(cor1, cor2, t);
}

QPointF QSvgAnimatedPropertyTransform::interpolatedSkew(uint index, qreal t) const
{
    QPointF skew1 = m_skews.at(index - 1);
    QPointF skew2 = m_skews.at(index);

    return pointInterpolator(skew1, skew2, t);
}

QPointF QSvgAnimatedPropertyTransform::interpolatedTranslation(uint index, qreal t) const
{
    QPointF t1 = m_translations.at(index - 1);
    QPointF t2 = m_translations.at(index);
    return pointInterpolator(t1, t2, t);
}

QPointF QSvgAnimatedPropertyTransform::interpolatedScale(uint index, qreal t) const
{
    QPointF s1 = m_scales.at(index - 1);
    QPointF s2 = m_scales.at(index);
    return pointInterpolator(s1, s2, t);
}

void QSvgAnimatedPropertyTransform::interpolate(uint index, qreal t)
{
    if (index >= (uint)m_keyFrames.size()) {
        qCWarning(lcSvgAnimatedProperty) << "Invalid index for key frames";
        return;
    }

    QTransform transform = QTransform();

    if (m_skews.size() == m_keyFrames.size()) {
        const QPointF skew = interpolatedSkew(index, t);
        transform.shear(qTan(qDegreesToRadians(skew.x())), qTan(qDegreesToRadians(skew.y())));
    }

    if (m_scales.size() == m_keyFrames.size()) {
        const QPointF scale = interpolatedScale(index, t);
        transform.scale(scale.x(), scale.y());
    }

    if (m_rotations.size() == m_keyFrames.size() &&
        m_centersOfRotation.size() == m_keyFrames.size()) {
        const qreal rotation = interpolatedRotation(index, t);
        const QPointF cor = interpolatedCenterOfRotation(index, t);

        transform.translate(cor.x(), cor.y());
        transform.rotate(rotation);
        transform.translate(-cor.x(), -cor.y());
    }

    if (m_translations.size() == m_keyFrames.size()) {
        const QPointF translation = interpolatedTranslation(index, t);
        transform.translate(translation.x(), translation.y());
    }

    m_interpolatedValue = transform;
}

QT_END_NAMESPACE
