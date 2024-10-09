// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATOR_P_H
#define QSVGANIMATOR_P_H

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
#include <QtSvg/private/qsvgnode_p.h>
#include "qsvgabstractanimation_p.h"

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAnimator
{
public:
    QSvgAnimator();
    ~QSvgAnimator();

    void appendAnimation(const QSvgNode *node, QSvgAbstractAnimation *anim);
    QList<QSvgAbstractAnimation *> animationsForNode(const QSvgNode *node) const;

    void advanceAnimations();
    void restartAnimation();
    qint64 currentElapsed();
    void setAnimationDuration(qint64 dur);
    qint64 animationDuration() const;
    void fastForwardAnimation(qint64 time);

    void applyAnimationsOnNode(const QSvgNode *node, QPainter *p);

private:
    QHash<const QSvgNode *, QList<QSvgAbstractAnimation *>> m_animations;
    qint64 m_time;
    qint64 m_animationDuration;
};

QT_END_NAMESPACE

#endif // QSVGANIMATOR_P_H
