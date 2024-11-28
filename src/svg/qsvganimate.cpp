// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvganimate_p.h"

QT_BEGIN_NAMESPACE

QSvgAnimateNode::QSvgAnimateNode(QSvgNode *parent)
    : QSvgNode(parent)
    , m_end(0)
    , m_fill(Fill::Freeze)
    , m_additive(Additive::Replace)
{
}

QSvgAnimateNode::~QSvgAnimateNode()
{

}

void QSvgAnimateNode::setLinkId(const QString &link)
{
    m_linkId = link;
}

const QString &QSvgAnimateNode::linkId() const
{
    return m_linkId;
}

QSvgAbstractAnimation::AnimationType QSvgAnimateNode::animationType() const
{
    return AnimationType::SMIL;
}

bool QSvgAnimateNode::isActive() const
{
    return !finished() || m_fill == Fill::Freeze;
}

void QSvgAnimateNode::setRunningTime(int startMs, int durMs, int endMs, int by)
{
    Q_UNUSED(by)
    m_start = startMs;
    m_end = endMs;
    m_duration = durMs;
}

void QSvgAnimateNode::setRepeatCount(qreal repeatCount)
{
    setIterationCount(repeatCount);
}

void QSvgAnimateNode::setFill(Fill fill)
{
    m_fill = fill;
}

QSvgAnimateNode::Fill QSvgAnimateNode::fill() const
{
    return m_fill;
}

void QSvgAnimateNode::setAdditiveType(Additive additive)
{
    m_additive = additive;
}

QSvgAnimateNode::Additive QSvgAnimateNode::additiveType() const
{
    return m_additive;
}

void QSvgAnimateNode::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    Q_UNUSED(p)
    Q_UNUSED(states)
}

bool QSvgAnimateNode::shouldDrawNode(QPainter *p, QSvgExtraStates &states) const
{
    Q_UNUSED(p)
    Q_UNUSED(states)
    return false;
}

QSvgAnimateColor::QSvgAnimateColor(QSvgNode *parent)
    : QSvgAnimateNode(parent)
{
}

QSvgNode::Type QSvgAnimateColor::type() const
{
    return QSvgNode::AnimateColor;
}

QSvgAnimateTransform::QSvgAnimateTransform(QSvgNode *parent)
    : QSvgAnimateNode(parent)
{
}

QSvgNode::Type QSvgAnimateTransform::type() const
{
    return Type::AnimateTransform;
}

QT_END_NAMESPACE
