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

    QSvgAbstractAnimatedProperty(QString name, Type type);
    virtual ~QSvgAbstractAnimatedProperty();

    void setKeyFrames(const QList<qreal> &keyFrames);
    QList<qreal> keyFrames() const;
    void setPropertyName(QString name);
    QStringView propertyName() const;
    Type type() const;
    QVariant interpolatedValue() const;
    virtual void interpolate(uint index, qreal t) = 0;

    static QSvgAbstractAnimatedProperty *createAnimatedProperty(QString name);
protected:
    QList<qreal> m_keyFrames;
    QVariant m_interpolatedValue;

private:
    QString m_propertyName;
    Type m_type;
};

class QSvgAnimatedPropertyColor : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyColor(QString name);

    void setColors(const QList<QColor> &colors);
    QList<QColor> colors() const;

    void interpolate(uint index, qreal t) override;

private:
    QList<QColor> m_colors;
};

class QSvgAnimatedPropertyTransform : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyTransform(QString name);

    void setTranslations(const QList<QPointF> &translations);
    QList<QPointF> translations() const;

    void setScales(const QList<QPointF> &scales);
    QList<QPointF> scales() const;

    void setRotations(const QList<qreal> &rotations);
    QList<qreal> rotations() const;

    void setCentersOfRotation(const QList<QPointF> &centersOfRotations);
    QList<QPointF> centersOfRotations() const;

    void setSkews(const QList<QPointF> &skews);
    QList<QPointF> skews() const;

    void interpolate(uint index, qreal t) override;

private:
    QList<QPointF> m_translations;
    QList<QPointF> m_scales;
    QList<qreal> m_rotations;
    QList<QPointF> m_centersOfRotation;
    QList<QPointF> m_skews;
};

QT_END_NAMESPACE

#endif // QSVGANIMATEDPROPERTY_P_H
