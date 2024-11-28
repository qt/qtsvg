// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATE_P_H
#define  QSVGANIMATE_P_H

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

#include "qsvgnode_p.h"
#include "private/qsvgabstractanimation_p.h"

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAnimateNode : public QSvgNode, public QSvgAbstractAnimation
{
public:
    enum Additive
    {
        Sum,
        Replace
    };
    enum Fill
    {
        Freeze,
        Remove
    };

public:
    QSvgAnimateNode(QSvgNode *parent = nullptr);
    virtual ~QSvgAnimateNode();

    void setLinkId(const QString &link);
    const QString &linkId() const;

    virtual AnimationType animationType() const override;
    virtual bool isActive() const override;

    void setRunningTime(int startMs, int durMs, int endMs, int by);
    void setRepeatCount(qreal repeatCount);

    void setFill(Fill fill);
    Fill fill() const;

    void setAdditiveType(Additive additive = Additive::Replace);
    Additive additiveType() const;

    virtual void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    virtual bool shouldDrawNode(QPainter *p, QSvgExtraStates &states) const override;

protected:
    qreal m_end;
    Fill m_fill;
    Additive m_additive;
    QString m_linkId;
};

class Q_SVG_EXPORT QSvgAnimateColor : public QSvgAnimateNode
{
public:
    QSvgAnimateColor(QSvgNode *parent = nullptr);
    virtual Type type() const override;
};

class Q_SVG_EXPORT QSvgAnimateTransform : public QSvgAnimateNode
{
public:
    QSvgAnimateTransform(QSvgNode *parent = nullptr);
    virtual Type type() const override;
};

QT_END_NAMESPACE

#endif // QSVGANIMATE_P_H
