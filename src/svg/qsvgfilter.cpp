// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgfilter_p.h"

#include "qsvgnode_p.h"
#include "qsvgtinydocument_p.h"
#include "qpainter.h"

#include <QLoggingCategory>
#include <QtGui/qimageiohandler.h>
#include <QVector4D>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSvgDraw);

QSvgFeFilterPrimitive::QSvgFeFilterPrimitive(QSvgNode *parent, QString input, QString result,
                                             const QSvgRectF &rect)
    : QSvgStructureNode(parent)
    , m_input(input)
    , m_result(result)
    , m_rect(rect)
{

}

QRectF QSvgFeFilterPrimitive::localFilterBoundingBox(QSvgNode *node,
                                                     const QRectF &itemBounds, const QRectF &filterBounds,
                                                     QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{

    QRectF localBounds;
    if (filterUnits != QtSvg::UnitTypes::userSpaceOnUse)
        localBounds = itemBounds;
    else
        localBounds = filterBounds;
    QRectF clipRect = m_rect.combinedWithLocalRect(localBounds, node->document()->viewBox(), primitiveUnits);
    clipRect = clipRect.intersected(filterBounds);

    return clipRect;
}

QRectF QSvgFeFilterPrimitive::globalFilterBoundingBox(QSvgNode *item, QPainter *p,
                                                      const QRectF &itemBounds, const QRectF &filterBounds,
                                                      QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    return p->transform().mapRect(localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits));
}

void QSvgFeFilterPrimitive::clipToTransformedBounds(QImage *buffer, QPainter *p, const QRectF &localRect) const
{
    QPainter painter(buffer);
    painter.setRenderHints(p->renderHints());
    painter.translate(-buffer->offset());
    QPainterPath clipPath;
    clipPath.setFillRule(Qt::OddEvenFill);
    clipPath.addRect(QRect(buffer->offset(), buffer->size()).adjusted(-10, -10, 20, 20));
    clipPath.addPolygon(p->transform().map(QPolygonF(localRect)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillPath(clipPath, Qt::transparent);
}

bool QSvgFeFilterPrimitive::requiresSourceAlpha() const
{
    return m_input == QLatin1StringView("SourceAlpha");
}

const QSvgFeFilterPrimitive *QSvgFeFilterPrimitive::castToFilterPrimitive(const QSvgNode *node)
{
    if (node->type() == QSvgNode::FeMerge ||
        node->type() == QSvgNode::FeColormatrix ||
        node->type() == QSvgNode::FeGaussianblur ||
        node->type() == QSvgNode::FeOffset ||
        node->type() == QSvgNode::FeComposite ||
        node->type() == QSvgNode::FeFlood ) {
        return reinterpret_cast<const QSvgFeFilterPrimitive*>(node);
    } else {
        return nullptr;
    }
}

QSvgFeColorMatrix::QSvgFeColorMatrix(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect,
                                     ColorShiftType type, Matrix matrix)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
    , m_type(type)
    , m_matrix(matrix)
{
    (void)m_type;
    //Magic numbers see SVG 1.1(Second edition)
    if (type == ColorShiftType::Saturate) {
        qreal s = qBound(0., matrix.data()[0], 1.);

        m_matrix.fill(0);

        m_matrix.data()[0+0*5] = 0.213f + 0.787f * s;
        m_matrix.data()[1+0*5] = 0.715f - 0.717f * s;
        m_matrix.data()[2+0*5] = 0.072f - 0.072f * s;

        m_matrix.data()[0+1*5] = 0.213f - 0.213f * s;
        m_matrix.data()[1+1*5] = 0.715f + 0.285f * s;
        m_matrix.data()[2+1*5] = 0.072f - 0.072f * s;

        m_matrix.data()[0+2*5] = 0.213f - 0.213f * s;
        m_matrix.data()[1+2*5] = 0.715f - 0.715f * s;
        m_matrix.data()[2+2*5] = 0.072f + 0.928f * s;

        m_matrix.data()[3+3*5] = 1;

    } else if (type == ColorShiftType::HueRotate){
        qreal angle = matrix.data()[0]/180.*M_PI;
        qreal s = sin(angle);
        qreal c = cos(angle);

        m_matrix.fill(0);

        QMatrix3x3 m1;
        m1.data()[0+0*3] = 0.213f;
        m1.data()[1+0*3] = 0.715f;
        m1.data()[2+0*3] = 0.072f;

        m1.data()[0+1*3] = 0.213f;
        m1.data()[1+1*3] = 0.715f;
        m1.data()[2+1*3] = 0.072f;

        m1.data()[0+2*3] = 0.213f;
        m1.data()[1+2*3] = 0.715f;
        m1.data()[2+2*3] = 0.072f;

        QMatrix3x3 m2;
        m2.data()[0+0*3] = 0.787 * c;
        m2.data()[1+0*3] = -0.715 * c;
        m2.data()[2+0*3] = -0.072 * c;

        m2.data()[0+1*3] = -0.213 * c;
        m2.data()[1+1*3] = 0.285 * c;
        m2.data()[2+1*3] = -0.072 * c;

        m2.data()[0+2*3] = -0.213 * c;
        m2.data()[1+2*3] = -0.715 * c;
        m2.data()[2+2*3] = 0.928 * c;

        QMatrix3x3 m3;
        m3.data()[0+0*3] = -0.213 * s;
        m3.data()[1+0*3] = -0.715 * s;
        m3.data()[2+0*3] = 0.928 * s;

        m3.data()[0+1*3] = 0.143 * s;
        m3.data()[1+1*3] = 0.140 * s;
        m3.data()[2+1*3] = -0.283 * s;

        m3.data()[0+2*3] = -0.787 * s;
        m3.data()[1+2*3] = 0.715 * s;
        m3.data()[2+2*3] = 0.072 * s;

        QMatrix3x3 m = m1 + m2 + m3;

        m_matrix.data()[0+0*5] = m.data()[0+0*3];
        m_matrix.data()[1+0*5] = m.data()[1+0*3];
        m_matrix.data()[2+0*5] = m.data()[2+0*3];

        m_matrix.data()[0+1*5] = m.data()[0+1*3];
        m_matrix.data()[1+1*5] = m.data()[1+1*3];
        m_matrix.data()[2+1*5] = m.data()[2+1*3];

        m_matrix.data()[0+2*5] = m.data()[0+2*3];
        m_matrix.data()[1+2*5] = m.data()[1+2*3];
        m_matrix.data()[2+2*5] = m.data()[2+2*3];

        m_matrix.data()[3+3*5] = 1;
    } else if (type == ColorShiftType::LuminanceToAlpha){
        m_matrix.fill(0);

        m_matrix.data()[0+3*5] = 0.2125;
        m_matrix.data()[1+3*5] = 0.7154;
        m_matrix.data()[2+3*5] = 0.0721;
    }
}

QSvgNode::Type QSvgFeColorMatrix::type() const
{
    return QSvgNode::FeColormatrix;
}

QImage QSvgFeColorMatrix::apply(QSvgNode *item, const QMap<QString, QImage> &sources, QPainter *p,
                                const QRectF &itemBounds, const QRectF &filterBounds,
                                QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    if (!sources.contains(m_input))
        return QImage();
    QImage source = sources[m_input];

    QRect clipRectGlob = globalFilterBoundingBox(item, p, itemBounds, filterBounds, primitiveUnits, filterUnits).toRect();
    QRect requiredRect = p->transform().mapRect(itemBounds).toRect();
    clipRectGlob = clipRectGlob.intersected(requiredRect);
    if (clipRectGlob.isEmpty())
        return QImage();

    QImage result;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(clipRectGlob.topLeft());
    result.fill(Qt::transparent);

    Q_ASSERT(source.depth() == 32);

    for (int i = 0; i < result.height(); i++) {
        int sourceI = i - source.offset().y() + result.offset().y();

        if (sourceI < 0 || sourceI >= source.height())
            continue;

        QRgb *sourceLine = reinterpret_cast<QRgb *>(source.scanLine(sourceI));
        QRgb *resultLine = reinterpret_cast<QRgb *>(result.scanLine(i));

        for (int j = 0; j < result.width(); j++) {
            int sourceJ = j - source.offset().x() + result.offset().x();

            if (sourceJ < 0 || sourceJ >= source.width())
                continue;

            qreal a = qAlpha(sourceLine[sourceJ]);
            qreal r = qBlue(sourceLine[sourceJ]);
            qreal g = qGreen(sourceLine[sourceJ]);
            qreal b = qRed(sourceLine[sourceJ]);

            qreal r2 = m_matrix.data()[0+0*5] * r +
                       m_matrix.data()[1+0*5] * g +
                       m_matrix.data()[2+0*5] * b +
                       m_matrix.data()[3+0*5] * a +
                       m_matrix.data()[4+0*5] * 255.;
            qreal g2 = m_matrix.data()[0+1*5] * r +
                       m_matrix.data()[1+1*5] * g +
                       m_matrix.data()[2+1*5] * b +
                       m_matrix.data()[3+1*5] * a +
                       m_matrix.data()[4+1*5] * 255.;
            qreal b2 = m_matrix.data()[0+2*5] * r +
                       m_matrix.data()[1+2*5] * g +
                       m_matrix.data()[2+2*5] * b +
                       m_matrix.data()[3+2*5] * a +
                       m_matrix.data()[4+2*5] * 255.;
            qreal a2 = m_matrix.data()[0+3*5] * r +
                       m_matrix.data()[1+3*5] * g +
                       m_matrix.data()[2+3*5] * b +
                       m_matrix.data()[3+3*5] * a +
                       m_matrix.data()[4+3*5] * 255.;

            resultLine[j] = qRgba(qBound(0, int(b2), 255),
                                  qBound(0, int(g2), 255),
                                  qBound(0, int(r2), 255),
                                  qBound(0, int(a2), 255));
        }
    }

    clipToTransformedBounds(&result, p, localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits));
    return result;
}

QSvgFeGaussianBlur::QSvgFeGaussianBlur(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect,
                                       qreal stdDeviationX, qreal stdDeviationY, EdgeMode edgemode)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
    , m_stdDeviationX(stdDeviationX)
    , m_stdDeviationY(stdDeviationY)
    , m_edgemode(edgemode)
{
    (void)m_edgemode;
}

QSvgNode::Type QSvgFeGaussianBlur::type() const
{
    return QSvgNode::FeGaussianblur;
}

QImage QSvgFeGaussianBlur::apply(QSvgNode *item, const QMap<QString, QImage> &sources, QPainter *p,
                                 const QRectF &itemBounds, const QRectF &filterBounds,
                                 QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    if (!sources.contains(m_input))
        return QImage();
    QImage source = sources[m_input];
    Q_ASSERT(source.depth() == 32);

    const qreal scaleX = qHypot(p->transform().m11(), p->transform().m21());
    const qreal scaleY = qHypot(p->transform().m12(), p->transform().m22());
    const QTransform scaleXr = QTransform::fromScale(scaleX, scaleY);
    const QTransform restXr = scaleXr.inverted() * p->transform();

    qreal sigma_x = scaleX * m_stdDeviationX;
    qreal sigma_y = scaleY * m_stdDeviationY;
    if (primitiveUnits == QtSvg::UnitTypes::objectBoundingBox) {
        sigma_x *= itemBounds.width();
        sigma_y *= itemBounds.height();
    }

    int dx = qMax(1, int(floor(sigma_x * 3. * sqrt(2. * M_PI) / 4. + 0.5)));
    int dy = qMax(1, int(floor(sigma_y * 3. * sqrt(2. * M_PI) / 4. + 0.5)));

    QRect clipRectGlob = scaleXr.mapRect(localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits)).toRect();
    QRect requiredRect = scaleXr.mapRect(itemBounds).toRect();
    requiredRect.adjust(- 3 * dx, -3 * dy, 3 * dx, 3 * dy);
    clipRectGlob = clipRectGlob.intersected(requiredRect);
    if (clipRectGlob.isEmpty())
        return QImage();

    QImage tempSource;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888_Premultiplied, &tempSource)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    tempSource.setOffset(clipRectGlob.topLeft());
    tempSource.fill(Qt::transparent);
    QPainter copyPainter(&tempSource);
    copyPainter.translate(-tempSource.offset());
    copyPainter.setTransform(restXr.inverted(), true);
    copyPainter.drawImage(source.offset(), source);
    copyPainter.end();

    QImage result = tempSource;

    // Using the approximation of a boxblur applied 3 times. Decoupling vertical and horizontal
    for (int m = 0; m < 6; m++) {
        QRgb *rawSource = reinterpret_cast<QRgb *>(tempSource.bits());
        QRgb *rawResult = reinterpret_cast<QRgb *>(result.bits());

        int d = (m % 2 == 0) ? dx : dy;
        int maxdim = (m % 2 == 0) ? tempSource.width() : tempSource.height();

        if (d < 1)
            continue;

        for (int i = 0; i < tempSource.width(); i++) {
            for (int j = 0; j < tempSource.height(); j++) {

                int iipos = (m % 2 == 0) ? i : j;

                QVector4D val(0, 0, 0, 0);
                for (int k = 0; k < d; k++) {
                    int ii = iipos + k - d / 2;
                    if (ii < 0 || ii >= maxdim)
                        continue;
                    QRgb rgbVal = (m % 2 == 0) ? rawSource[ii + j * tempSource.width()] : rawSource[i + ii * tempSource.width()];
                    val += QVector4D(qBlue(rgbVal), //TODO: Why are values switched here???
                                     qGreen(rgbVal),
                                     qRed(rgbVal), //TODO: Why are values switched here???
                                     qAlpha(rgbVal)) / d;
                }
                rawResult[i + j * tempSource.width()] = qRgba(qBound(0, int(val[0]), 255),
                                                              qBound(0, int(val[1]), 255),
                                                              qBound(0, int(val[2]), 255),
                                                              qBound(0, int(val[3]), 255));
            }
        }
        tempSource = result;
    }

    QRectF trueClipRectGlob = globalFilterBoundingBox(item, p, itemBounds, filterBounds, primitiveUnits, filterUnits);

    result = QImage();
    if (!QImageIOHandler::allocateImage(trueClipRectGlob.toRect().size(), QImage::Format_RGBA8888_Premultiplied, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(trueClipRectGlob.toRect().topLeft());
    result.fill(Qt::transparent);
    QPainter transformPainter(&result);
    transformPainter.setRenderHint(QPainter::Antialiasing, true);

    transformPainter.translate(-result.offset());
    transformPainter.setTransform(restXr, true);
    transformPainter.drawImage(clipRectGlob.topLeft(), tempSource);
    transformPainter.end();

    clipToTransformedBounds(&result, p, localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits));
    return result;
}

QSvgFeOffset::QSvgFeOffset(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect,
                           qreal dx, qreal dy)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
    , m_dx(dx)
    , m_dy(dy)
{

}

QSvgNode::Type QSvgFeOffset::type() const
{
    return QSvgNode::FeOffset;
}

QImage QSvgFeOffset::apply(QSvgNode *item, const QMap<QString, QImage> &sources, QPainter *p,
                           const QRectF &itemBounds, const QRectF &filterBounds,
                           QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    if (!sources.contains(m_input))
        return QImage();

    const QImage &source = sources[m_input];

    QRectF clipRect = localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits);
    QRect clipRectGlob = p->transform().mapRect(clipRect).toRect();

    QPoint offset(m_dx, m_dy);
    if (primitiveUnits == QtSvg::UnitTypes::objectBoundingBox) {
        offset = QPoint(m_dx * itemBounds.width(),
                        m_dy * itemBounds.height());
    }
    offset = p->transform().map(offset) - p->transform().map(QPoint(0, 0));

    QRect requiredRect = QRect(source.offset(), source.size()).translated(offset);
    clipRectGlob = clipRectGlob.intersected(requiredRect);

    if (clipRectGlob.isEmpty())
        return QImage();

    QImage result;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(clipRectGlob.topLeft());
    result.fill(Qt::transparent);

    QPainter copyPainter(&result);
    copyPainter.drawImage(source.offset()
                        - result.offset() + offset, source);
    copyPainter.end();

    clipToTransformedBounds(&result, p, clipRect);
    return result;
}


QSvgFeMerge::QSvgFeMerge(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
{

}

QSvgNode::Type QSvgFeMerge::type() const
{
    return QSvgNode::FeMerge;
}

QImage QSvgFeMerge::apply(QSvgNode *item, const QMap<QString, QImage> &sources, QPainter *p,
                          const QRectF &itemBounds, const QRectF &filterBounds,
                          QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    QList<QImage> mergeNodeResults;
    QRect requiredRect;

    for (int i = 0; i < renderers().size(); i++) {
        QSvgNode *child = renderers().at(i);
        if (child->type() == QSvgNode::FeMergenode) {
            QSvgFeMergeNode *filter = static_cast<QSvgFeMergeNode*>(child);
            mergeNodeResults.append(filter->apply(item, sources, p, itemBounds, filterBounds, primitiveUnits, filterUnits));
            requiredRect = requiredRect.united(QRect(mergeNodeResults.last().offset(),
                                                     mergeNodeResults.last().size()));
        }
    }

    QRectF clipRect = localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits);
    QRect clipRectGlob = p->transform().mapRect(clipRect).toRect();
    clipRectGlob = clipRectGlob.intersected(requiredRect);
    if (clipRectGlob.isEmpty())
        return QImage();

    QImage result;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(clipRectGlob.topLeft());
    result.fill(Qt::transparent);

    QPainter proxyPainter(&result);
    for (const QImage &i : mergeNodeResults) {
        proxyPainter.drawImage(QRect(i.offset() - result.offset(), i.size()), i);
    }
    proxyPainter.end();

    clipToTransformedBounds(&result, p, clipRect);
    return result;
}

bool QSvgFeMerge::requiresSourceAlpha() const
{
    for (int i = 0; i < renderers().size(); i++) {
        QSvgNode *child = renderers().at(i);
        if (child->type() == QSvgNode::FeMergenode) {
            QSvgFeMergeNode *filter = static_cast<QSvgFeMergeNode *>(child);
            if (filter->requiresSourceAlpha())
                return true;
        }
    }
    return false;
}

QSvgFeMergeNode::QSvgFeMergeNode(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
{

}

QSvgNode::Type QSvgFeMergeNode::type() const
{
    return QSvgNode::FeMergenode;
}

QImage QSvgFeMergeNode::apply(QSvgNode *, const QMap<QString, QImage> &sources, QPainter *,
                              const QRectF &, const QRectF &, QtSvg::UnitTypes, QtSvg::UnitTypes) const
{
    return sources.value(m_input);
}

QSvgFeComposite::QSvgFeComposite(QSvgNode *parent, QString input, QString result, const QSvgRectF &rect,
                                 QString input2, Operator op, QVector4D k)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
    , m_input2(input2)
    , m_operator(op)
    , m_k(k)
{

}

QSvgNode::Type QSvgFeComposite::type() const
{
    return QSvgNode::FeComposite;
}

QImage QSvgFeComposite::apply(QSvgNode *item, const QMap<QString, QImage> &sources, QPainter *p,
                              const QRectF &itemBounds, const QRectF &filterBounds,
                              QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{
    if (!sources.contains(m_input))
        return QImage();
    if (!sources.contains(m_input2))
        return QImage();
    QImage source1 = sources[m_input];
    QImage source2 = sources[m_input2];
    Q_ASSERT(source1.depth() == 32);
    Q_ASSERT(source2.depth() == 32);

    QRectF clipRect = localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits);
    QRect clipRectGlob = globalFilterBoundingBox(item, p, itemBounds, filterBounds, primitiveUnits, filterUnits).toRect();
    QRect requiredRect = QRect(source1.offset(), source1.size()).united(
                         QRect(source2.offset(), source2.size()));
    clipRectGlob = clipRectGlob.intersected(requiredRect);
    if (clipRectGlob.isEmpty())
        return QImage();

    QImage result;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(clipRectGlob.topLeft());
    result.fill(Qt::transparent);

    if (m_operator == Operator::Arithmetic) {
        const qreal k1 = m_k.x();
        const qreal k2 = m_k.y();
        const qreal k3 = m_k.z();
        const qreal k4 = m_k.w();

        for (int j = 0; j < result.height(); j++) {
            int jj1 = j - source1.offset().y() + result.offset().y();
            int jj2 = j - source2.offset().y() + result.offset().y();

            QRgb *resultLine = reinterpret_cast<QRgb *>(result.scanLine(j));
            QRgb *source1Line = nullptr;
            QRgb *source2Line = nullptr;

            if (jj1 >= 0 && jj1 < source1.size().height())
                source1Line = reinterpret_cast<QRgb *>(source1.scanLine(jj1));
            if (jj2 >= 0 && jj2 < source2.size().height())
                 source2Line = reinterpret_cast<QRgb *>(source2.scanLine(jj2));

            for (int i = 0; i < result.width(); i++) {
                int ii1 = i - source1.offset().x() + result.offset().x();
                int ii2 = i - source2.offset().x() + result.offset().x();

                QVector4D s1 = QVector4D(0, 0, 0, 0);
                QVector4D s2 = QVector4D(0, 0, 0, 0);

                if (ii1 >= 0 && ii1 < source1.size().width() && source1Line) {
                    QRgb pixel1 = source1Line[ii1];
                    s1 = QVector4D(qRed(pixel1),
                                   qGreen(pixel1),
                                   qBlue(pixel1),
                                   qAlpha(pixel1));
                }

                if (ii2 >= 0 && ii2 < source2.size().width() && source2Line) {
                    QRgb pixel2 = source2Line[ii2];
                    s2 = QVector4D(qRed(pixel2),
                                   qGreen(pixel2),
                                   qBlue(pixel2),
                                   qAlpha(pixel2));
                }

                int r = k1 * s1.x() * s2.x() / 255. + k2 * s1.x() + k3 * s2.x() + k4 * 255.;
                int g = k1 * s1.y() * s2.y() / 255. + k2 * s1.y() + k3 * s2.y() + k4 * 255.;
                int b = k1 * s1.z() * s2.z() / 255. + k2 * s1.z() + k3 * s2.z() + k4 * 255.;
                int a = k1 * s1.w() * s2.w() / 255. + k2 * s1.w() + k3 * s2.w() + k4 * 255.;

                qreal alpha = qBound(0, a, 255) / 255.;
                if (alpha == 0)
                    alpha = 1;
                resultLine[i] =  qRgba(qBound(0., r / alpha, 255.),
                                       qBound(0., g / alpha, 255.),
                                       qBound(0., b / alpha, 255.),
                                       qBound(0, a, 255));
            }
        }
    } else {
        QPainter proxyPainter(&result);
        proxyPainter.drawImage(QRect(source1.offset() - result.offset(), source1.size()), source1);

        switch (m_operator) {
        case Operator::In:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            break;
        case Operator::Out:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            break;
        case Operator::Xor:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_Xor);
            break;
        case Operator::Lighter:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_Lighten);
            break;
        case Operator::Atop:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_DestinationAtop);
            break;
        case Operator::Over:
            proxyPainter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
            break;
        case Operator::Arithmetic: // handled above
            Q_UNREACHABLE();
            break;
        }
        proxyPainter.drawImage(QRect(source2.offset()-result.offset(), source2.size()), source2);
        proxyPainter.end();
    }

    clipToTransformedBounds(&result, p, clipRect);
    return result;
}

bool QSvgFeComposite::requiresSourceAlpha() const
{
    if (QSvgFeFilterPrimitive::requiresSourceAlpha())
        return true;
    return m_input2 == QLatin1StringView("SourceAlpha");
}


QSvgFeFlood::QSvgFeFlood(QSvgNode *parent, QString input, QString result,
                         const QSvgRectF &rect, const QColor &color)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
    , m_color(color)
{

}

QSvgNode::Type QSvgFeFlood::type() const
{
    return QSvgNode::FeFlood;
}

QImage QSvgFeFlood::apply(QSvgNode *item, const QMap<QString, QImage> &,
                          QPainter *p, const QRectF &itemBounds, const QRectF &filterBounds,
                          QtSvg::UnitTypes primitiveUnits, QtSvg::UnitTypes filterUnits) const
{

    QRectF clipRect = localFilterBoundingBox(item, itemBounds, filterBounds, primitiveUnits, filterUnits);
    QRect clipRectGlob = p->transform().mapRect(clipRect).toRect();

    QImage result;
    if (!QImageIOHandler::allocateImage(clipRectGlob.size(), QImage::Format_RGBA8888, &result)) {
        qCWarning(lcSvgDraw) << "The requested filter buffer is too big, ignoring";
        return QImage();
    }
    result.setOffset(clipRectGlob.topLeft());
    result.fill(m_color);

    clipToTransformedBounds(&result, p, clipRect);
    return result;
}

QSvgFeUnsupported::QSvgFeUnsupported(QSvgNode *parent, QString input, QString result,
                         const QSvgRectF &rect)
    : QSvgFeFilterPrimitive(parent, input, result, rect)
{
}

QSvgNode::Type QSvgFeUnsupported::type() const
{
    return QSvgNode::FeUnsupported;
}

QImage QSvgFeUnsupported::apply(QSvgNode *, const QMap<QString, QImage> &,
                          QPainter *, const QRectF &, const QRectF &,
                          QtSvg::UnitTypes, QtSvg::UnitTypes) const
{
    qCDebug(lcSvgDraw) <<"Unsupported filter primitive should not be applied.";
    return QImage();
}

QT_END_NAMESPACE
