// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGCSSHANDLER_P_H
#define QSVGCSSHANDLER_P_H

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
#include <QStringView>
#include <QList>
#include <QtGui/private/qcssparser_p.h>
#include <QtSvg/private/qsvgcssanimation_p.h>

QT_BEGIN_NAMESPACE

class QSvgCssHandler {
public:
    QSvgCssHandler() = default;

    QSvgCssAnimation *createAnimation(const QString &name);
    void collectAnimations(const QCss::StyleSheet &sheet);

private:
    void updateColorProperty(const QCss::Declaration &decl, QSvgAnimatedPropertyColor *property);
    void updateTransformProperty(const QCss::Declaration &decl, QSvgAnimatedPropertyTransform *property);

private:
    QHash<QString, QCss::AnimationRule> m_animations;
    QHash<QString, QSvgCssAnimation *> m_cachedAnimations;
};


QT_END_NAMESPACE

#endif // QSVGCSSHANDLER_P_H
