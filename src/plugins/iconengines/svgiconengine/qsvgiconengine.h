/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QSVGICONENGINE_H
#define QSVGICONENGINE_H

#include <QtGui/qiconengine.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QSvgIconEnginePrivate;

class QSvgIconEngine : public QIconEngine
{
public:
    QSvgIconEngine();
    QSvgIconEngine(const QSvgIconEngine &other);
    ~QSvgIconEngine();
    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode,
                     QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override;

    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                   QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size,
                 QIcon::Mode mode, QIcon::State state) override;

    QString key() const override;
    QIconEngine *clone() const override;
    bool read(QDataStream &in) override;
    bool write(QDataStream &out) const override;

    void virtual_hook(int id, void *data) override;
private:
    QSharedDataPointer<QSvgIconEnginePrivate> d;
};

QT_END_NAMESPACE

#endif
