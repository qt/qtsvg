// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSVGPAINTSERVER_P_H
#define QSVGPAINTSERVER_P_H

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

#include <memory>
#include <QtSvg/private/qtsvgglobal_p.h>
#include <QtCore/qrect.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtGui/qcolor.h>
#include <QtGui/qbrush.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpen.h>
#include <QtGui/qtransform.h>

QT_BEGIN_NAMESPACE

class QSvgNode;
class QSvgDocument;
class QSvgPattern;
struct QSvgExtraStates;

class Q_SVG_EXPORT QSvgPaintServer
{
    Q_DISABLE_COPY_MOVE(QSvgPaintServer)
public:
    enum class Type : qint8 {
        SolidColor,
        Gradient,
        Pattern,
    };
public:
    QSvgPaintServer() = default;
    virtual ~QSvgPaintServer();
    virtual QBrush brush(QPainter *p, const QSvgNode *node, QSvgExtraStates &states) = 0;
    virtual Type type() const = 0;
};

class Q_SVG_EXPORT QSvgSolidColorPaint : public QSvgPaintServer
{
public:
    QSvgSolidColorPaint(const QColor &color);
    ~QSvgSolidColorPaint() override;
    Type type() const override;

    const QColor & qcolor() const;

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override;

private:
    // solid-color       v 	x 	'inherit' | <SVGColor.datatype>
    // solid-opacity     v 	x 	'inherit' | <OpacityValue.datatype>
    QColor m_solidColor;

    QBrush m_oldFill;
    QPen m_oldStroke;
};

inline QSvgPaintServer::Type QSvgSolidColorPaint::type() const
{
    return Type::SolidColor;
}

inline const QColor &QSvgSolidColorPaint::qcolor() const
{
    return m_solidColor;
}

inline QBrush QSvgSolidColorPaint::brush(QPainter *, const QSvgNode *, QSvgExtraStates &)
{
    return m_solidColor;
}


class Q_SVG_EXPORT QSvgGradientPaint : public QSvgPaintServer
{
public:
    QSvgGradientPaint(std::unique_ptr<QGradient> grad);
    ~QSvgGradientPaint() override;
    Type type() const override;

    void setStopLink(const QString &link, QSvgDocument *doc);
    QString stopLink() const;

    void setTransform(const QTransform &transform);
    QTransform qtransform() const;

    QGradient *qgradient();

    bool gradientStopsSet() const;

    void setGradientStopsSet(bool set);

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override;
private:
    void resolveStops();
    void resolveStops_helper(QSet<QString> &visited);
private:
    std::unique_ptr<QGradient> m_gradient;
    QTransform m_transform;

    QSvgDocument *m_doc{nullptr};
    QString m_link;
    bool m_gradientStopsSet{false};
};

inline QSvgPaintServer::Type QSvgGradientPaint::type() const
{
    return Type::Gradient;
}

inline void QSvgGradientPaint::setStopLink(const QString &link, QSvgDocument *doc)
{
    m_link = link;
    m_doc  = doc;
}

inline QString QSvgGradientPaint::stopLink() const
{
    return m_link;
}

inline void QSvgGradientPaint::setTransform(const QTransform &transform)
{
    m_transform = transform;
}

inline QTransform QSvgGradientPaint::qtransform() const
{
    return m_transform;
}

inline QGradient *QSvgGradientPaint::qgradient()
{
    return m_gradient.get();
}

inline void QSvgGradientPaint::setGradientStopsSet(bool set)
{
    m_gradientStopsSet = set;
}

inline bool QSvgGradientPaint::gradientStopsSet() const
{
    return m_gradientStopsSet;
}

class Q_SVG_EXPORT QSvgPatternPaint : public QSvgPaintServer
{
public:
    QSvgPatternPaint(QSvgPattern *pattern);
    ~QSvgPatternPaint() override;
    Type type() const override;

    QBrush brush(QPainter *, const QSvgNode *, QSvgExtraStates &) override;
    QSvgPattern *patternNode();
private:
    QSvgPattern *m_pattern;
    QRectF m_parentBound;
};

inline QSvgPaintServer::Type QSvgPatternPaint::type() const
{
    return Type::Pattern;
}

inline QSvgPattern *QSvgPatternPaint::patternNode()
{
    return m_pattern;
}

using QSvgPaintServerSharedPtr = std::shared_ptr<QSvgPaintServer>;
using QSvgGradientPaintSharedPtr = std::shared_ptr<QSvgGradientPaint>;

QT_END_NAMESPACE

#endif // QSVGPAINTSERVER_P_H
