// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvganimator_p.h"
#include "QtCore/qdatetime.h"

QT_BEGIN_NAMESPACE

QSvgAnimator::QSvgAnimator()
    : m_time(0)
    , m_animationDuration(0)
{
}

QSvgAnimator::~QSvgAnimator()
{
}

void QSvgAnimator::restartAnimation()
{
    m_time = QDateTime::currentMSecsSinceEpoch();
}

qint64 QSvgAnimator::currentElapsed()
{
    return QDateTime::currentMSecsSinceEpoch() - m_time;
}

void QSvgAnimator::setAnimationDuration(qint64 dur)
{
    m_animationDuration = dur;
}

qint64 QSvgAnimator::animationDuration() const
{
    return m_animationDuration;
}

void QSvgAnimator::fastForwardAnimation(qint64 time)
{
    m_time += time;
}

QT_END_NAMESPACE
