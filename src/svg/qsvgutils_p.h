// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSVGUTILS_P_H
#define QSVGUTILS_P_H

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

#include <QtSvg/qtsvgglobal.h>
#include <QtCore/qrect.h>
#include <cmath>

QT_BEGIN_NAMESPACE

namespace QSvgUtils {
// Safely casts a qreal value to int while validating the range
// before casting to avoid UB.
inline int castToInt(qreal v)
{
    if (std::isnan(v))
        return 0;

    const qreal boundedValue = std::clamp(v,
                                          qreal(std::numeric_limits<int>::min()),
                                          qreal(std::numeric_limits<int>::max()));
    return int(std::round(boundedValue));
}

// Same as QRectF::toAlignedRect but safely cast to int
inline QRect toAlignedRect(const QRectF &r) {
    const qreal xmin = std::floor(r.x());
    const qreal xmax = std::ceil(r.x() + r.width());
    const qreal ymin = std::floor(r.y());
    const qreal ymax = std::ceil(r.y() + r.height());
    return QRect(castToInt(xmin), castToInt(ymin),
                 castToInt(xmax - xmin), castToInt(ymax - ymin));
}
} // namespace QSvgUtils

QT_END_NAMESPACE
#endif //QSVGUTILS_P_H
