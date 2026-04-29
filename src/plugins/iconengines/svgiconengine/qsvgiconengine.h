// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtGui/qiconengine.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QSvgIconEnginePrivate;

class QSvgIconEngine : public QIconEngine
{
protected:
    QSvgIconEngine(const QSvgIconEngine &other);
public:
    QSvgIconEngine();
    ~QSvgIconEngine();

    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode,
                     QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode,
                         QIcon::State state, qreal scale) override;

    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                   QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size,
                 QIcon::Mode mode, QIcon::State state) override;

    bool isNull() override;
    QString key() const override;
    QSvgIconEngine *clone() const override;
    bool read(QDataStream &in) override;
    bool write(QDataStream &out) const override;
private:
    QSharedDataPointer<QSvgIconEnginePrivate> d;
};

QT_END_NAMESPACE
