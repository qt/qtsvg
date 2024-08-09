// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgtinydocument_p.h"

#include "qsvghandler_p.h"
#include "qsvgfont_p.h"

#include "qpainter.h"
#include "qfile.h"
#include "qbuffer.h"
#include "qbytearray.h"
#include "qqueue.h"
#include "qstack.h"
#include "qtransform.h"
#include "qdebug.h"

#ifndef QT_NO_COMPRESS
#include <zlib.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QSvgTinyDocument::QSvgTinyDocument(QtSvg::Options options)
    : QSvgStructureNode(0)
    , m_widthPercent(false)
    , m_heightPercent(false)
    , m_time(0)
    , m_animated(false)
    , m_animationDuration(0)
    , m_fps(30)
    , m_options(options)
{
}

QSvgTinyDocument::~QSvgTinyDocument()
{
}

static bool hasSvgHeader(const QByteArray &buf)
{
    QTextStream s(buf); // Handle multi-byte encodings
    QString h = s.readAll();
    QStringView th = QStringView(h).trimmed();
    bool matched = false;
    if (th.startsWith("<svg"_L1) || th.startsWith("<!DOCTYPE svg"_L1))
        matched = true;
    else if (th.startsWith("<?xml"_L1) || th.startsWith("<!--"_L1))
        matched = th.contains("<!DOCTYPE svg"_L1) || th.contains("<svg"_L1);
    return matched;
}

#ifndef QT_NO_COMPRESS
static QByteArray qt_inflateSvgzDataFrom(QIODevice *device, bool doCheckContent = true);
#   ifdef QT_BUILD_INTERNAL
Q_AUTOTEST_EXPORT QByteArray qt_inflateGZipDataFrom(QIODevice *device)
{
    return qt_inflateSvgzDataFrom(device, false); // autotest wants unchecked result
}
#   endif

static QByteArray qt_inflateSvgzDataFrom(QIODevice *device, bool doCheckContent)
{
    if (!device)
        return QByteArray();

    if (!device->isOpen())
        device->open(QIODevice::ReadOnly);

    Q_ASSERT(device->isOpen() && device->isReadable());

    static const int CHUNK_SIZE = 4096;
    int zlibResult = Z_OK;

    QByteArray source;
    QByteArray destination;

    // Initialize zlib stream struct
    z_stream zlibStream;
    zlibStream.next_in = Z_NULL;
    zlibStream.avail_in = 0;
    zlibStream.avail_out = 0;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;

    // Adding 16 to the window size gives us gzip decoding
    if (inflateInit2(&zlibStream, MAX_WBITS + 16) != Z_OK) {
        qCWarning(lcSvgHandler, "Cannot initialize zlib, because: %s",
                (zlibStream.msg != NULL ? zlibStream.msg : "Unknown error"));
        return QByteArray();
    }

    bool stillMoreWorkToDo = true;
    while (stillMoreWorkToDo) {

        if (!zlibStream.avail_in) {
            source = device->read(CHUNK_SIZE);

            if (source.isEmpty())
                break;

            zlibStream.avail_in = source.size();
            zlibStream.next_in = reinterpret_cast<Bytef*>(source.data());
        }

        do {
            // Prepare the destination buffer
            int oldSize = destination.size();
            if (oldSize > INT_MAX - CHUNK_SIZE) {
                inflateEnd(&zlibStream);
                qCWarning(lcSvgHandler, "Error while inflating gzip file: integer size overflow");
                return QByteArray();
            }

            destination.resize(oldSize + CHUNK_SIZE);
            zlibStream.next_out = reinterpret_cast<Bytef*>(
                    destination.data() + oldSize - zlibStream.avail_out);
            zlibStream.avail_out += CHUNK_SIZE;

            zlibResult = inflate(&zlibStream, Z_NO_FLUSH);
            switch (zlibResult) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_STREAM_ERROR:
                case Z_MEM_ERROR: {
                    inflateEnd(&zlibStream);
                    qCWarning(lcSvgHandler, "Error while inflating gzip file: %s",
                            (zlibStream.msg != NULL ? zlibStream.msg : "Unknown error"));
                    return QByteArray();
                }
            }

        // If the output buffer still has more room after calling inflate
        // it means we have to provide more data, so exit the loop here
        } while (!zlibStream.avail_out);

        if (doCheckContent) {
            // Quick format check, equivalent to QSvgIOHandler::canRead()
            if (!hasSvgHeader(destination)) {
                inflateEnd(&zlibStream);
                qCWarning(lcSvgHandler, "Error while inflating gzip file: SVG format check failed");
                return QByteArray();
            }
            doCheckContent = false; // Run only once, on first chunk
        }

        if (zlibResult == Z_STREAM_END) {
            // Make sure there are no more members to process before exiting
            if (!(zlibStream.avail_in && inflateReset(&zlibStream) == Z_OK))
                stillMoreWorkToDo = false;
        }
    }

    // Chop off trailing space in the buffer
    destination.chop(zlibStream.avail_out);

    inflateEnd(&zlibStream);
    return destination;
}
#else
static QByteArray qt_inflateSvgzDataFrom(QIODevice *)
{
    return QByteArray();
}
#endif

QSvgTinyDocument *QSvgTinyDocument::load(const QString &fileName, QtSvg::Options options)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(lcSvgHandler, "Cannot open file '%s', because: %s",
                  qPrintable(fileName), qPrintable(file.errorString()));
        return 0;
    }

    if (fileName.endsWith(QLatin1String(".svgz"), Qt::CaseInsensitive)
            || fileName.endsWith(QLatin1String(".svg.gz"), Qt::CaseInsensitive)) {
        return load(qt_inflateSvgzDataFrom(&file));
    }

    QSvgTinyDocument *doc = nullptr;
    QSvgHandler handler(&file, options);
    if (handler.ok()) {
        doc = handler.document();
        doc->m_animationDuration = handler.animationDuration();
    } else {
        qCWarning(lcSvgHandler, "Cannot read file '%s', because: %s (line %d)",
                 qPrintable(fileName), qPrintable(handler.errorString()), handler.lineNumber());
        delete handler.document();
    }
    return doc;
}

QSvgTinyDocument *QSvgTinyDocument::load(const QByteArray &contents, QtSvg::Options options)
{
    QByteArray svg;
    // Check for gzip magic number and inflate if appropriate
    if (contents.startsWith("\x1f\x8b")) {
        QBuffer buffer;
        buffer.setData(contents);
        svg = qt_inflateSvgzDataFrom(&buffer);
    } else {
        svg = contents;
    }
    if (svg.isNull())
        return nullptr;

    QBuffer buffer;
    buffer.setData(svg);
    buffer.open(QIODevice::ReadOnly);
    QSvgHandler handler(&buffer, options);

    QSvgTinyDocument *doc = nullptr;
    if (handler.ok()) {
        doc = handler.document();
        doc->m_animationDuration = handler.animationDuration();
    } else {
        delete handler.document();
    }
    return doc;
}

QSvgTinyDocument *QSvgTinyDocument::load(QXmlStreamReader *contents, QtSvg::Options options)
{
    QSvgHandler handler(contents, options);

    QSvgTinyDocument *doc = nullptr;
    if (handler.ok()) {
        doc = handler.document();
        doc->m_animationDuration = handler.animationDuration();
    } else {
        delete handler.document();
    }
    return doc;
}

void QSvgTinyDocument::draw(QPainter *p, const QRectF &bounds)
{
    if (m_time == 0)
        m_time = QDateTime::currentMSecsSinceEpoch();

    if (displayMode() == QSvgNode::NoneMode)
        return;

    p->save();
    //sets default style on the painter
    //### not the most optimal way
    mapSourceToTarget(p, bounds);
    initPainter(p);
    QList<QSvgNode*>::iterator itr = m_renderers.begin();
    applyStyle(p, m_states);
    while (itr != m_renderers.end()) {
        QSvgNode *node = *itr;
        if ((node->isVisible()) && (node->displayMode() != QSvgNode::NoneMode))
            node->draw(p, m_states);
        ++itr;
    }
    revertStyle(p, m_states);
    p->restore();
}


void QSvgTinyDocument::draw(QPainter *p, const QString &id,
                            const QRectF &bounds)
{
    QSvgNode *node = scopeNode(id);

    if (!node) {
        qCDebug(lcSvgHandler, "Couldn't find node %s. Skipping rendering.", qPrintable(id));
        return;
    }
    if (m_time == 0)
        m_time = QDateTime::currentMSecsSinceEpoch();

    if (node->displayMode() == QSvgNode::NoneMode)
        return;

    p->save();

    const QRectF elementBounds = node->bounds();

    mapSourceToTarget(p, bounds, elementBounds);
    QTransform originalTransform = p->worldTransform();

    //XXX set default style on the painter
    QPen pen(Qt::NoBrush, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
    pen.setMiterLimit(4);
    p->setPen(pen);
    p->setBrush(Qt::black);
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);

    QStack<QSvgNode*> parentApplyStack;
    QSvgNode *parent = node->parent();
    while (parent) {
        parentApplyStack.push(parent);
        parent = parent->parent();
    }

    for (int i = parentApplyStack.size() - 1; i >= 0; --i)
        parentApplyStack[i]->applyStyle(p, m_states);

    // Reset the world transform so that our parents don't affect
    // the position
    QTransform currentTransform = p->worldTransform();
    p->setWorldTransform(originalTransform);

    node->draw(p, m_states);

    p->setWorldTransform(currentTransform);

    for (int i = 0; i < parentApplyStack.size(); ++i)
        parentApplyStack[i]->revertStyle(p, m_states);

    //p->fillRect(bounds.adjusted(-5, -5, 5, 5), QColor(0, 0, 255, 100));

    p->restore();
}

QSvgNode::Type QSvgTinyDocument::type() const
{
    return Doc;
}

void QSvgTinyDocument::setWidth(int len, bool percent)
{
    m_size.setWidth(len);
    m_widthPercent = percent;
}

void QSvgTinyDocument::setHeight(int len, bool percent)
{
    m_size.setHeight(len);
    m_heightPercent = percent;
}

void QSvgTinyDocument::setPreserveAspectRatio(bool on)
{
    m_preserveAspectRatio = on;
}

void QSvgTinyDocument::setViewBox(const QRectF &rect)
{
    m_viewBox = rect;
    m_implicitViewBox = rect.isNull();
}

QtSvg::Options QSvgTinyDocument::options() const
{
    return m_options;
}

void QSvgTinyDocument::addSvgFont(QSvgFont *font)
{
    m_fonts.insert(font->familyName(), font);
}

QSvgFont * QSvgTinyDocument::svgFont(const QString &family) const
{
    return m_fonts[family];
}

void QSvgTinyDocument::addNamedNode(const QString &id, QSvgNode *node)
{
    m_namedNodes.insert(id, node);
}

QSvgNode *QSvgTinyDocument::namedNode(const QString &id) const
{
    return m_namedNodes.value(id);
}

void QSvgTinyDocument::addNamedStyle(const QString &id, QSvgPaintStyleProperty *style)
{
    if (!m_namedStyles.contains(id))
        m_namedStyles.insert(id, style);
    else
        qCWarning(lcSvgHandler) << "Duplicate unique style id:" << id;
}

QSvgPaintStyleProperty *QSvgTinyDocument::namedStyle(const QString &id) const
{
    return m_namedStyles.value(id);
}

void QSvgTinyDocument::restartAnimation()
{
    m_time = QDateTime::currentMSecsSinceEpoch();
}

bool QSvgTinyDocument::animated() const
{
    return m_animated;
}

void QSvgTinyDocument::setAnimated(bool a)
{
    m_animated = a;
}

void QSvgTinyDocument::draw(QPainter *p)
{
    draw(p, QRectF());
}

void QSvgTinyDocument::drawCommand(QPainter *, QSvgExtraStates &)
{
    qCDebug(lcSvgHandler) << "SVG Tiny does not support nested <svg> elements: ignored.";
    return;
}

static bool isValidMatrix(const QTransform &transform)
{
    qreal determinant = transform.determinant();
    return qIsFinite(determinant);
}

void QSvgTinyDocument::mapSourceToTarget(QPainter *p, const QRectF &targetRect, const QRectF &sourceRect)
{
    QTransform oldTransform = p->worldTransform();

    QRectF target = targetRect;
    if (target.isEmpty()) {
        QPaintDevice *dev = p->device();
        QRectF deviceRect(0, 0, dev->width(), dev->height());
        if (deviceRect.isEmpty()) {
            if (sourceRect.isEmpty())
                target = QRectF(QPointF(0, 0), size());
            else
                target = QRectF(QPointF(0, 0), sourceRect.size());
        } else {
            target = deviceRect;
        }
    }

    QRectF source = sourceRect;
    if (source.isEmpty())
        source = viewBox();

    if (source != target && !qFuzzyIsNull(source.width()) && !qFuzzyIsNull(source.height())) {
        if (m_implicitViewBox || !preserveAspectRatio()) {
            // Code path used when no view box is set, or IgnoreAspectRatio requested
            QTransform transform;
            transform.scale(target.width() / source.width(),
                            target.height() / source.height());
            QRectF c2 = transform.mapRect(source);
            p->translate(target.x() - c2.x(),
                         target.y() - c2.y());
            p->scale(target.width() / source.width(),
                     target.height() / source.height());
        } else {
            // Code path used when KeepAspectRatio is requested. This attempts to emulate the default values
            // of the <preserveAspectRatio tag that's implicitly defined when <viewbox> is used.

            // Scale the view box into the view port (target) by preserve the aspect ratio.
            QSizeF viewBoxSize = source.size();
            viewBoxSize.scale(target.width(), target.height(), Qt::KeepAspectRatio);

            // Center the view box in the view port
            p->translate(target.x() + (target.width() - viewBoxSize.width()) / 2,
                         target.y() + (target.height() - viewBoxSize.height()) / 2);

            p->scale(viewBoxSize.width() / source.width(),
                     viewBoxSize.height() / source.height());

            // Apply the view box translation if specified.
            p->translate(-source.x(), -source.y());
        }
    }

    if (!isValidMatrix(p->worldTransform()))
        p->setWorldTransform(oldTransform);
}

QRectF QSvgTinyDocument::boundsOnElement(const QString &id) const
{
    const QSvgNode *node = scopeNode(id);
    if (!node)
        node = this;
    return node->bounds();
}

bool QSvgTinyDocument::elementExists(const QString &id) const
{
    QSvgNode *node = scopeNode(id);

    return (node!=0);
}

QTransform QSvgTinyDocument::transformForElement(const QString &id) const
{
    QSvgNode *node = scopeNode(id);

    if (!node) {
        qCDebug(lcSvgHandler, "Couldn't find node %s. Skipping rendering.", qPrintable(id));
        return QTransform();
    }

    QTransform t;

    node = node->parent();
    while (node) {
        if (node->m_style.transform)
            t *= node->m_style.transform->qtransform();
        node = node->parent();
    }

    return t;
}

int QSvgTinyDocument::currentFrame() const
{
    double runningPercentage = qMin(currentElapsed() / double(m_animationDuration), 1.);

    int totalFrames = m_fps * m_animationDuration;

    return int(runningPercentage * totalFrames);
}

void QSvgTinyDocument::setCurrentFrame(int frame)
{
    int totalFrames = m_fps * m_animationDuration;
    double framePercentage = frame/double(totalFrames);
    double timeForFrame = m_animationDuration * framePercentage; //in S
    timeForFrame *= 1000; //in ms
    int timeToAdd = int(timeForFrame - currentElapsed());
    m_time += timeToAdd;
}

void QSvgTinyDocument::setFramesPerSecond(int num)
{
    m_fps = num;
}

bool QSvgTinyDocument::isLikelySvg(QIODevice *device, bool *isCompressed)
{
    constexpr int bufSize = 4096;
    char buf[bufSize];
    char inflateBuf[bufSize];
    bool useInflateBuf = false;
    int readLen = device->peek(buf, bufSize);
    if (readLen < 8)
        return false;
#ifndef QT_NO_COMPRESS
    if (quint8(buf[0]) == 0x1f && quint8(buf[1]) == 0x8b) {
        // Indicates gzip compressed content, i.e. svgz
        z_stream zlibStream;
        zlibStream.avail_in = readLen;
        zlibStream.next_out = reinterpret_cast<Bytef *>(inflateBuf);
        zlibStream.avail_out = bufSize;
        zlibStream.next_in = reinterpret_cast<Bytef *>(buf);
        zlibStream.zalloc = Z_NULL;
        zlibStream.zfree = Z_NULL;
        zlibStream.opaque = Z_NULL;
        if (inflateInit2(&zlibStream, MAX_WBITS + 16) != Z_OK)
            return false;
        int zlibResult = inflate(&zlibStream, Z_NO_FLUSH);
        inflateEnd(&zlibStream);
        if ((zlibResult != Z_OK && zlibResult != Z_STREAM_END) || zlibStream.total_out < 8)
            return false;
        readLen = zlibStream.total_out;
        if (isCompressed)
            *isCompressed = true;
        useInflateBuf = true;
    }
#endif
    return hasSvgHeader(QByteArray::fromRawData(useInflateBuf ? inflateBuf : buf, readLen));
}

QT_END_NAMESPACE
