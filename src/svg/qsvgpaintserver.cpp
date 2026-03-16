// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsvgpaintserver_p.h"
#include <QtSvg/private/qsvgnode_p.h>
#include <QtSvg/private/qsvgdocument_p.h>

QT_BEGIN_NAMESPACE

QSvgPaintServer::~QSvgPaintServer()
    = default;

QSvgSolidColorPaint::QSvgSolidColorPaint(const QColor &color)
    : m_solidColor(color)
{
}

QSvgSolidColorPaint::~QSvgSolidColorPaint()
    = default;

QSvgGradientPaint::QSvgGradientPaint(std::unique_ptr<QGradient> grad)
    : m_gradient(std::move(grad))
{
}

QSvgGradientPaint::~QSvgGradientPaint()
    = default;

QBrush QSvgGradientPaint::brush(QPainter *, const QSvgNode *, QSvgExtraStates &)
{
    if (!m_link.isEmpty())
        resolveStops();

    // If the gradient is marked as empty, insert transparent black
    if (!m_gradientStopsSet) {
        m_gradient->setStops(QGradientStops() << QGradientStop(0.0, QColor(0, 0, 0, 0)));
        m_gradientStopsSet = true;
    }

    QBrush b(*m_gradient);

    if (!m_transform.isIdentity())
        b.setTransform(m_transform);

    return b;
}

void QSvgGradientPaint::resolveStops()
{
    QSet<QString> visited;
    resolveStops_helper(visited);
}

void QSvgGradientPaint::resolveStops_helper(QSet<QString> &visited)
{
    if (!m_link.isEmpty() && m_doc) {
        QSvgPaintServerSharedPtr paintServer = m_doc->paintServer(m_link);
        if (paintServer && !visited.contains(m_link)) {
            visited.insert(m_link);
            if (paintServer->type() == QSvgPaintServer::Type::Gradient) {
                QSvgGradientPaint *st =
                    static_cast<QSvgGradientPaint*>(paintServer.get());
                st->resolveStops_helper(visited);
                m_gradient->setStops(st->qgradient()->stops());
                m_gradientStopsSet = st->gradientStopsSet();
            }
        } else {
            qWarning("Could not resolve property : %s", qPrintable(m_link));
        }
        m_link.clear();
    }
}

QSvgPatternPaint::QSvgPatternPaint(QSvgPattern *pattern)
    : m_pattern(pattern)
{
}

QSvgPatternPaint::~QSvgPatternPaint()
    = default;

QBrush QSvgPatternPaint::brush(QPainter *p, const QSvgNode *node, QSvgExtraStates &states)
{
    QBrush b(m_pattern->patternImage(p, states, node));
    b.setTransform(m_pattern->appliedTransform());
    return b;
}

QT_END_NAMESPACE
