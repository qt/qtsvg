/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSVGIOHANDLER_H
#define QSVGIOHANDLER_H

#include <QtGui/qimageiohandler.h>

#ifndef QT_NO_SVGRENDERER

QT_BEGIN_NAMESPACE

class QImage;
class QByteArray;
class QIODevice;
class QVariant;
class QSvgIOHandlerPrivate;

class QSvgIOHandler : public QImageIOHandler
{
public:
    QSvgIOHandler();
    ~QSvgIOHandler();
    bool canRead() const override;
    bool read(QImage *image) override;
    static bool canRead(QIODevice *device);
    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant & value) override;
    bool supportsOption(ImageOption option) const override;

private:
    QSvgIOHandlerPrivate *d;
};

QT_END_NAMESPACE

#endif // QT_NO_SVGRENDERER
#endif // QSVGIOHANDLER_H
