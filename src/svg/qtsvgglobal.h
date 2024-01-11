// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSVGGLOBAL_H
#define QTSVGGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtSvg/qtsvgexports.h>

QT_BEGIN_NAMESPACE

namespace QtSvg {

enum class FeatureSet : quint32
{
    StaticTiny1_2,
    AllAvailable = 0xffffffff,
};

}

QT_END_NAMESPACE

#endif
