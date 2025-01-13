// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATEDPROPERTY_P_H
#define QSVGANIMATEDPROPERTY_P_H

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

#include <QtSvg/private/qtsvgglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#include <QtCore/qpoint.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAbstractAnimatedProperty
{
public:
    enum Type
    {
        Int,
        Float,
        Color,
        Transform,
    };

    QSvgAbstractAnimatedProperty(const QString &name, Type type);
    virtual ~QSvgAbstractAnimatedProperty();

    void setKeyFrames(const QList<qreal> &keyFrames);
    void appendKeyFrame(qreal keyFrame);
    QList<qreal> keyFrames() const;
    void setPropertyName(const QString &name);
    QStringView propertyName() const;
    Type type() const;
    QVariant interpolatedValue() const;
    virtual void interpolate(uint index, qreal t) = 0;

    static QSvgAbstractAnimatedProperty *createAnimatedProperty(const QString &name);
protected:
    QList<qreal> m_keyFrames;
    QVariant m_interpolatedValue;

private:
    QString m_propertyName;
    Type m_type;
};

class Q_SVG_EXPORT QSvgAnimatedPropertyColor : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyColor(const QString &name);

    void setColors(const QList<QColor> &colors);
    void appendColor(const QColor &color);
    QList<QColor> colors() const;

    void interpolate(uint index, qreal t) override;

private:
    QList<QColor> m_colors;
};

class Q_SVG_EXPORT QSvgAnimatedPropertyTransform : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyTransform(const QString &name);

    void setTranslations(const QList<QPointF> &translations);
    void appendTranslation(const QPointF &translation);
    QList<QPointF> translations() const;

    void setScales(const QList<QPointF> &scales);
    void appendScale(const QPointF &scale);
    QList<QPointF> scales() const;

    void setRotations(const QList<qreal> &rotations);
    void appendRotation(qreal rotation);
    QList<qreal> rotations() const;

    void setCentersOfRotation(const QList<QPointF> &centersOfRotations);
    void appendCenterOfRotation(const QPointF &centerOfRotation);
    QList<QPointF> centersOfRotations() const;

    void setSkews(const QList<QPointF> &skews);
    void appendSkew(const QPointF &skew);
    QList<QPointF> skews() const;

    void interpolate(uint index, qreal t) override;

    qreal interpolatedRotation(uint index, qreal t) const;
    QPointF interpolatedCenterOfRotation(uint index, qreal t) const;
    QPointF interpolatedScale(uint index, qreal t) const;
    QPointF interpolatedTranslation(uint index, qreal t) const;
    QPointF interpolatedSkew(uint index, qreal t) const;

private:
    QList<QPointF> m_translations;
    QList<QPointF> m_scales;
    QList<qreal> m_rotations;
    QList<QPointF> m_centersOfRotation;
    QList<QPointF> m_skews;
};

QT_END_NAMESPACE

#endif // QSVGANIMATEDPROPERTY_P_H
