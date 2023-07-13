// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGHELPER_P_H
#define QSVGHELPER_P_H

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

#include "qtsvgglobal_p.h"

#include <QRectF>

QT_BEGIN_NAMESPACE

class Q_SVG_PRIVATE_EXPORT QSvgRectF : public QRectF
{
public:
    QSvgRectF(const QRectF &r = QRectF(),
              QSvg::UnitTypes unitX = QSvg::UnitTypes::userSpaceOnUse,
              QSvg::UnitTypes unitY = QSvg::UnitTypes::userSpaceOnUse,
              QSvg::UnitTypes unitW = QSvg::UnitTypes::userSpaceOnUse,
              QSvg::UnitTypes unitH = QSvg::UnitTypes::userSpaceOnUse)
        : QRectF(r)
        , m_unitX(unitX)
        , m_unitY(unitY)
        , m_unitW(unitW)
        , m_unitH(unitH)
    {}

    QRectF combineWithLocalRect(const QRectF &localRect) const {
        QRectF result;
        if (m_unitX == QSvg::UnitTypes::objectBoundingBox)
            result.setX(localRect.x() + x() * localRect.width());
        else
            result.setX(x());
        if (m_unitY == QSvg::UnitTypes::objectBoundingBox)
            result.setY(localRect.y() + y() * localRect.height());
        else
            result.setY(y());
        if (m_unitW == QSvg::UnitTypes::objectBoundingBox)
            result.setWidth(localRect.width() * width());
        else
            result.setWidth(width());
        if (m_unitH == QSvg::UnitTypes::objectBoundingBox)
            result.setHeight(localRect.height() * height());
        else
            result.setHeight(height());
        return result;
    }

    QRectF combineWithLocalRect(const QRectF &localRect, QSvg::UnitTypes units) const {
        QRectF result;
        if (m_unitX == QSvg::UnitTypes::objectBoundingBox || units == QSvg::UnitTypes::objectBoundingBox)
            result.setX(localRect.x() + x() * localRect.width());
        else
            result.setX(x());
        if (m_unitY == QSvg::UnitTypes::objectBoundingBox || units == QSvg::UnitTypes::objectBoundingBox)
            result.setY(localRect.y() + y() * localRect.height());
        else
            result.setY(y());
        if (m_unitW == QSvg::UnitTypes::objectBoundingBox || units == QSvg::UnitTypes::objectBoundingBox)
            result.setWidth(localRect.width() * width());
        else
            result.setWidth(width());
        if (m_unitH == QSvg::UnitTypes::objectBoundingBox || units == QSvg::UnitTypes::objectBoundingBox)
            result.setHeight(localRect.height() * height());
        else
            result.setHeight(height());
        return result;
    }

    QSvg::UnitTypes unitX() const {return m_unitX;}
    QSvg::UnitTypes unitY() const {return m_unitY;}
    QSvg::UnitTypes unitW() const {return m_unitW;}
    QSvg::UnitTypes unitH() const {return m_unitH;}

    void setUnitX(QSvg::UnitTypes unit) {m_unitX = unit;}
    void setUnitY(QSvg::UnitTypes unit) {m_unitY = unit;}
    void setUnitW(QSvg::UnitTypes unit) {m_unitW = unit;}
    void setUnitH(QSvg::UnitTypes unit) {m_unitH = unit;}

protected:
    QSvg::UnitTypes m_unitX,
                    m_unitY,
                    m_unitW,
                    m_unitH;
};

QT_END_NAMESPACE

#endif // QSVGHELPER_P_H
