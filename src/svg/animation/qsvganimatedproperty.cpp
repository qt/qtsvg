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

static qreal lerp(qreal a, qreal b, qreal t)
{
    return a + (b - a) * t;
}

static QPointF pointInterpolator(QPointF v1, QPointF v2, qreal t)
{
    qreal x = lerp(v1.x(), v2.x(), t);
    qreal y = lerp(v1.y(), v2.y(), t);

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

QList<QColor> QSvgAnimatedPropertyColor::colors() const
{
    return m_colors;
}

void QSvgAnimatedPropertyColor::interpolate(uint index, qreal t)
{
    QColor c1 = m_colors.at(index - 1);
    QColor c2 = m_colors.at(index);

    int alpha  = lerp(c1.alpha(), c2.alpha(), t);
    int red    = lerp(c1.red(), c2.red(), t);
    int green  = lerp(c1.green(), c2.green(), t);
    int blue   = lerp(c1.blue(), c2.blue(), t);

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

QList<QPointF> QSvgAnimatedPropertyTransform::translations() const
{
    return m_translations;
}

void QSvgAnimatedPropertyTransform::setScales(const QList<QPointF> &scales)
{
    m_scales = scales;
}

QList<QPointF> QSvgAnimatedPropertyTransform::scales() const
{
    return m_scales;
}

void QSvgAnimatedPropertyTransform::setRotations(const QList<qreal> &rotations)
{
    m_rotations = rotations;
}

QList<qreal> QSvgAnimatedPropertyTransform::rotations() const
{
    return m_rotations;
}

void QSvgAnimatedPropertyTransform::setCentersOfRotation(const QList<QPointF> &centersOfRotations)
{
    m_centersOfRotation = centersOfRotations;
}

QList<QPointF> QSvgAnimatedPropertyTransform::centersOfRotations() const
{
    return m_centersOfRotation;
}

void QSvgAnimatedPropertyTransform::setSkews(const QList<QPointF> &skews)
{
    m_skews = skews;
}

QList<QPointF> QSvgAnimatedPropertyTransform::skews() const
{
    return m_skews;
}

void QSvgAnimatedPropertyTransform::interpolate(uint index, qreal t)
{
    if (index >= (uint)m_keyFrames.size()) {
        qCWarning(lcSvgAnimatedProperty) << "Invalid index for key frames";
        return;
    }

    QTransform transform = QTransform();

    if (m_skews.size() == m_keyFrames.size()) {
        QPointF skew1 = m_skews.at(index - 1);
        QPointF skew2 = m_skews.at(index);

        QPointF skew = pointInterpolator(skew1, skew2, t);
        transform.shear(qTan(qDegreesToRadians(skew.x())), qTan(qDegreesToRadians(skew.y())));
    }

    if (m_scales.size() == m_keyFrames.size()) {
        QPointF s1 = m_scales.at(index - 1);
        QPointF s2 = m_scales.at(index);
        QPointF scale = pointInterpolator(s1, s2, t);
        transform.scale(scale.x(), scale.y());
    }

    if (m_rotations.size() == m_keyFrames.size() &&
        m_centersOfRotation.size() == m_keyFrames.size()) {
        qreal r1 = m_rotations.at(index - 1);
        QPointF cor1 = m_centersOfRotation.at(index - 1);
        qreal r2 = m_rotations.at(index);
        QPointF cor2 = m_centersOfRotation.at(index);
        qreal rotation  = lerp(r1, r2, t);
        QPointF cor = pointInterpolator(cor1, cor2, t);

        transform.translate(cor.x(), cor.y());
        transform.rotate(rotation);
        transform.translate(-cor.x(), -cor.y());
    }

    if (m_translations.size() == m_keyFrames.size()) {
        QPointF t1 = m_translations.at(index - 1);
        QPointF t2 = m_translations.at(index);
        QPointF translation  = pointInterpolator(t1, t2, t);

        transform.translate(translation.x(), translation.y());
    }

    m_interpolatedValue = transform;
}

QT_END_NAMESPACE
