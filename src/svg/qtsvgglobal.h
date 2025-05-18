// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QTSVGGLOBAL_H
#define QTSVGGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtSvg/qtsvgexports.h>

QT_BEGIN_NAMESPACE

namespace QtSvg {

enum Option : quint32 {
    NoOption           = 0x00,
    Tiny12FeaturesOnly = 0x01,
    AssumeTrustedSource = 0x02,
};
Q_DECLARE_FLAGS(Options, Option)
Q_DECLARE_OPERATORS_FOR_FLAGS(Options)

}

QT_END_NAMESPACE

#endif
