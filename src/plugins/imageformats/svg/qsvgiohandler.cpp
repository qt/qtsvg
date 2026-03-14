// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qsvgiohandler.h"

#ifndef QT_NO_SVGRENDERER

#include "qsvgrenderer.h"
#include "private/qsvgdocument_p.h"
#include "qimage.h"
#include "qpixmap.h"
#include "qpainter.h"
#include "qvariant.h"
#include "qbuffer.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSvgIOHandlerPrivate
{
public:
    QSvgIOHandlerPrivate(QSvgIOHandler *qq)
        : q(qq)
    {}

    bool load(QIODevice *device);

    QSvgIOHandler   *q = nullptr;
    QSvgRenderer     r;
    QXmlStreamReader xmlReader;
    QSize            defaultSize;
    QRect            clipRect;
    QSize            scaledSize;
    QRect            scaledClipRect;
    bool             loadAttempted = false;
    bool             loadStatus = false;
    bool             readDone = false;
    int              currentFrame = 0;
    int              frameCount = 0;
    int              frameDelay = 0;
    QColor           backColor = Qt::transparent;
};


bool QSvgIOHandlerPrivate::load(QIODevice *device)
{
    if (loadAttempted)
        return loadStatus;
    loadAttempted = true;
    if (q->format().isEmpty())
        q->canRead();

    // # The SVG renderer doesn't handle trailing, unrelated data, so we must
    // assume that all available data in the device is to be read.
    bool res = false;
    QBuffer *buf = qobject_cast<QBuffer *>(device);
    if (buf) {
        const QByteArray &ba = buf->data();
        res = r.load(QByteArray::fromRawData(ba.constData() + buf->pos(), ba.size() - buf->pos()));
        buf->seek(ba.size());
#ifndef QT_NO_COMPRESS
    } else if (q->format() == "svgz") {
        res = r.load(device->readAll());
#endif
    } else {
        xmlReader.setDevice(device);
        res = r.load(&xmlReader);
    }

    if (res) {
        defaultSize = r.defaultSize();
        loadStatus = true;
        if (r.animated()) {
            const int duration = r.animationDuration();
            const int fps = r.framesPerSecond();
            frameCount = qMax(1, static_cast<int>(qint64(duration) * fps / 1000));
            frameDelay = fps > 0 ? 1000 / fps : 0;
        }
    }

    return loadStatus;
}



QSvgIOHandler::QSvgIOHandler()
    : d(new QSvgIOHandlerPrivate(this))
{

}


QSvgIOHandler::~QSvgIOHandler()
{
    delete d;
}

bool QSvgIOHandler::canRead() const
{
    if (!device())
        return false;

    if (d->loadAttempted) {
        if (!d->loadStatus)
            return false;
        if (d->r.animated())
            return d->currentFrame < d->frameCount;
        return !d->readDone;
    }

    // Not yet loaded — probe the device to determine format
    bool isCompressed = false;
    if (QSvgDocument::isLikelySvg(device(), &isCompressed)) {
        setFormat(isCompressed ? "svgz" : "svg");
        return true;
    }
    return false;
}

bool QSvgIOHandler::read(QImage *image)
{
    if (!d->load(device()))
        return false;

    // For non-animated SVGs, preserve original single-read behavior
    if (!d->r.animated()) {
        if (d->readDone)
            return false;
    } else {
        // For animated SVGs, set the current frame on the renderer
        if (d->currentFrame >= d->frameCount)
            return false;
        d->r.setCurrentFrame(d->currentFrame);
    }

    bool xform = (d->clipRect.isValid() || d->scaledSize.isValid() || d->scaledClipRect.isValid());
    QSize finalSize = d->defaultSize;
    QRectF bounds;
    if (xform && !d->defaultSize.isEmpty()) {
        bounds = QRectF(QPointF(0,0), QSizeF(d->defaultSize));
        QPoint tr1, tr2;
        QSizeF sc(1, 1);
        if (d->clipRect.isValid()) {
            tr1 = -d->clipRect.topLeft();
            finalSize = d->clipRect.size();
        }
        if (d->scaledSize.isValid()) {
            sc = QSizeF(qreal(d->scaledSize.width()) / finalSize.width(),
                        qreal(d->scaledSize.height()) / finalSize.height());
            finalSize = d->scaledSize;
        }
        if (d->scaledClipRect.isValid()) {
            tr2 = -d->scaledClipRect.topLeft();
            finalSize = d->scaledClipRect.size();
        }
        QTransform t;
        t.translate(tr2.x(), tr2.y());
        t.scale(sc.width(), sc.height());
        t.translate(tr1.x(), tr1.y());
        bounds = t.mapRect(bounds);
    }
    if (finalSize.isEmpty()) {
        *image = QImage();
    } else {
        if (qMax(finalSize.width(), finalSize.height()) > 0xffff)
            return false; // Assume corrupted file
        if (!QImageIOHandler::allocateImage(finalSize, QImage::Format_ARGB32_Premultiplied, image))
            return false;
        image->fill(d->backColor.rgba());
        QPainter p(image);
        d->r.render(&p, bounds);
        p.end();
    }

    d->readDone = true;
    if (d->r.animated())
        ++d->currentFrame;
    return true;
}


QVariant QSvgIOHandler::option(ImageOption option) const
{
    switch(option) {
    case ImageFormat:
        return QImage::Format_ARGB32_Premultiplied;
        break;
    case Size:
        d->load(device());
        return d->defaultSize;
        break;
    case ClipRect:
        return d->clipRect;
        break;
    case ScaledSize:
        return d->scaledSize;
        break;
    case ScaledClipRect:
        return d->scaledClipRect;
        break;
    case BackgroundColor:
        return d->backColor;
        break;
    case Animation:
        d->load(device());
        return d->r.animated();
        break;
    default:
        break;
    }
    return QVariant();
}


void QSvgIOHandler::setOption(ImageOption option, const QVariant & value)
{
    switch(option) {
    case ClipRect:
        d->clipRect = value.toRect();
        break;
    case ScaledSize:
        d->scaledSize = value.toSize();
        break;
    case ScaledClipRect:
        d->scaledClipRect = value.toRect();
        break;
    case BackgroundColor:
        d->backColor = value.value<QColor>();
        break;
    default:
        break;
    }
}


bool QSvgIOHandler::supportsOption(ImageOption option) const
{
    switch(option)
    {
    case ImageFormat:
    case Size:
    case ClipRect:
    case ScaledSize:
    case ScaledClipRect:
    case BackgroundColor:
    case Animation:
        return true;
    default:
        break;
    }
    return false;
}


bool QSvgIOHandler::jumpToNextImage()
{
    return jumpToImage(d->currentFrame + 1);
}

bool QSvgIOHandler::jumpToImage(int imageNumber)
{
    if (!d->load(device()) || !d->r.animated())
        return false;
    if (imageNumber < 0 || imageNumber >= d->frameCount)
        return false;
    d->currentFrame = imageNumber;
    return true;
}

int QSvgIOHandler::loopCount() const
{
    return 0;
}

int QSvgIOHandler::imageCount() const
{
    return d->frameCount;
}

int QSvgIOHandler::nextImageDelay() const
{
    return d->frameDelay;
}

int QSvgIOHandler::currentImageNumber() const
{
    if (d->r.animated())
        return d->readDone ? d->currentFrame : -1;
    return 0;
}


bool QSvgIOHandler::canRead(QIODevice *device)
{
    return QSvgDocument::isLikelySvg(device);
}

QT_END_NAMESPACE

#endif // QT_NO_SVGRENDERER
