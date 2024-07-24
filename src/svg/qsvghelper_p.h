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
              QtSvg::UnitTypes unitX = QtSvg::UnitTypes::userSpaceOnUse,
              QtSvg::UnitTypes unitY = QtSvg::UnitTypes::userSpaceOnUse,
              QtSvg::UnitTypes unitW = QtSvg::UnitTypes::userSpaceOnUse,
              QtSvg::UnitTypes unitH = QtSvg::UnitTypes::userSpaceOnUse)
        : QRectF(r)
        , m_unitX(unitX)
        , m_unitY(unitY)
        , m_unitW(unitW)
        , m_unitH(unitH)
    {}

    QRectF combinedWithLocalRect(const QRectF &localRect) const {
        QRectF result;
        if (m_unitX == QtSvg::UnitTypes::objectBoundingBox)
            result.setX(localRect.x() + x() * localRect.width());
        else
            result.setX(x());
        if (m_unitY == QtSvg::UnitTypes::objectBoundingBox)
            result.setY(localRect.y() + y() * localRect.height());
        else
            result.setY(y());
        if (m_unitW == QtSvg::UnitTypes::objectBoundingBox)
            result.setWidth(localRect.width() * width());
        else
            result.setWidth(width());
        if (m_unitH == QtSvg::UnitTypes::objectBoundingBox)
            result.setHeight(localRect.height() * height());
        else
            result.setHeight(height());
        return result;
    }

    QPointF translationRelativeToBoundingBox(const QRectF &boundingBox) const {
        QPointF result;

        if (m_unitX == QtSvg::UnitTypes::objectBoundingBox)
            result.setX(x() * boundingBox.width());
        else
            result.setX(x());
        if (m_unitY == QtSvg::UnitTypes::objectBoundingBox)
            result.setY(y() * boundingBox.height());
        else
            result.setY(y());
        return result;
    }

    QRectF combinedWithLocalRect(const QRectF &localRect, const QRectF &canvasRect, QtSvg::UnitTypes units) const {
        QRectF result;
        if (units == QtSvg::UnitTypes::objectBoundingBox)
            result.setX(localRect.x() + x() * localRect.width());
        else if (m_unitX == QtSvg::UnitTypes::objectBoundingBox)
            result.setX(localRect.x() + x() * canvasRect.width());
        else
            result.setX(x());
        if (units == QtSvg::UnitTypes::objectBoundingBox)
            result.setY(localRect.y() + y() * localRect.height());
        else if (m_unitY == QtSvg::UnitTypes::objectBoundingBox)
            result.setY(localRect.y() + y() * canvasRect.height());
        else
            result.setY(y());
        if (units == QtSvg::UnitTypes::objectBoundingBox)
            result.setWidth(localRect.width() * width());
        else if (m_unitW == QtSvg::UnitTypes::objectBoundingBox)
            result.setWidth(canvasRect.width() * width());
        else
            result.setWidth(width());
        if (units == QtSvg::UnitTypes::objectBoundingBox)
            result.setHeight(localRect.height() * height());
        else if (m_unitH == QtSvg::UnitTypes::objectBoundingBox)
            result.setHeight(canvasRect.height() * height());
        else
            result.setHeight(height());

        return result;
    }

    QtSvg::UnitTypes unitX() const {return m_unitX;}
    QtSvg::UnitTypes unitY() const {return m_unitY;}
    QtSvg::UnitTypes unitW() const {return m_unitW;}
    QtSvg::UnitTypes unitH() const {return m_unitH;}

    void setUnitX(QtSvg::UnitTypes unit) {m_unitX = unit;}
    void setUnitY(QtSvg::UnitTypes unit) {m_unitY = unit;}
    void setUnitW(QtSvg::UnitTypes unit) {m_unitW = unit;}
    void setUnitH(QtSvg::UnitTypes unit) {m_unitH = unit;}

protected:
    QtSvg::UnitTypes m_unitX,
                     m_unitY,
                     m_unitW,
                     m_unitH;
};

QT_END_NAMESPACE

#endif // QSVGHELPER_P_H
