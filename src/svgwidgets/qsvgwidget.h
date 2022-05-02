/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt SVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QSVGWIDGET_H
#define QSVGWIDGET_H

#include <QtSvgWidgets/qtsvgwidgetsglobal.h>
#include <QtWidgets/qwidget.h>


QT_BEGIN_NAMESPACE


class QSvgWidgetPrivate;
class QPaintEvent;
class QSvgRenderer;

class Q_SVGWIDGETS_EXPORT QSvgWidget : public QWidget
{
    Q_OBJECT
public:
    QSvgWidget(QWidget *parent = nullptr);
    QSvgWidget(const QString &file, QWidget *parent = nullptr);
    ~QSvgWidget();

    QSvgRenderer *renderer() const;

    QSize sizeHint() const override;
public Q_SLOTS:
    void load(const QString &file);
    void load(const QByteArray &contents);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    Q_DISABLE_COPY(QSvgWidget)
    Q_DECLARE_PRIVATE(QSvgWidget)
};

QT_END_NAMESPACE

#endif // QSVGWIDGET_H
