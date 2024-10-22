// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgfont_p.h"

#include "qpainter.h"
#include "qpen.h"
#include "qdebug.h"
#include "qpicture.h"

QT_BEGIN_NAMESPACE

QSvgGlyph::QSvgGlyph(QChar unicode, const QPainterPath &path, qreal horizAdvX)
    : m_unicode(unicode), m_path(path), m_horizAdvX(horizAdvX)
{

}


QSvgFont::QSvgFont(qreal horizAdvX)
    : m_horizAdvX(horizAdvX)
{
}


QString QSvgFont::familyName() const
{
    return m_familyName;
}


void QSvgFont::addGlyph(QChar unicode, const QPainterPath &path, qreal horizAdvX )
{
    m_glyphs.insert(unicode, QSvgGlyph(unicode, path,
                                       (horizAdvX==-1)?m_horizAdvX:horizAdvX));
}


void QSvgFont::draw(QPainter *p, const QPointF &point, const QString &str,
                    qreal pixelSize, Qt::Alignment alignment) const
{
    draw_helper(p, point, str, pixelSize, alignment, nullptr);
}

QRectF QSvgFont::boundingRect(QPainter *p, const QPointF &point, const QString &str,
                              qreal pixelSize, Qt::Alignment alignment) const
{
    QRectF bounds;
    draw_helper(p, point, str, pixelSize, alignment, &bounds);
    return bounds;
}

void QSvgFont::draw_helper(QPainter *p, const QPointF &point, const QString &str, qreal pixelSize,
                           Qt::Alignment alignment, QRectF *boundingRect) const
{
    const bool isPainting = (boundingRect == nullptr);

    p->save();
    p->translate(point);
    p->scale(pixelSize / m_unitsPerEm, -pixelSize / m_unitsPerEm);

    // Calculate the text width to be used for alignment
    int textWidth = 0;
    QString::const_iterator itr = str.constBegin();
    for ( ; itr != str.constEnd(); ++itr) {
        QChar unicode = *itr;
        if (!m_glyphs.contains(*itr)) {
            unicode = u'\0';
            if (!m_glyphs.contains(unicode))
                continue;
        }
        textWidth += static_cast<int>(m_glyphs[unicode].m_horizAdvX);
    }

    QPoint alignmentOffset(0, 0);
    if (alignment == Qt::AlignHCenter) {
        alignmentOffset.setX(-textWidth / 2);
    } else if (alignment == Qt::AlignRight) {
        alignmentOffset.setX(-textWidth);
    }

    p->translate(alignmentOffset);

    // since in SVG the embedded font ain't really a path
    // the outline has got to stay untransformed...
    qreal penWidth = p->pen().widthF();
    penWidth /= (pixelSize/m_unitsPerEm);
    QPen pen = p->pen();
    pen.setWidthF(penWidth);
    p->setPen(pen);

    itr = str.constBegin();
    for ( ; itr != str.constEnd(); ++itr) {
        QChar unicode = *itr;
        if (!m_glyphs.contains(*itr)) {
            unicode = u'\0';
            if (!m_glyphs.contains(unicode))
                continue;
        }

        if (isPainting)
            p->drawPath(m_glyphs[unicode].m_path);

        if (boundingRect) {
            QPainterPathStroker stroker;
            stroker.setWidth(penWidth);
            stroker.setJoinStyle(p->pen().joinStyle());
            stroker.setMiterLimit(p->pen().miterLimit());
            QPainterPath stroke = stroker.createStroke(m_glyphs[unicode].m_path);
            *boundingRect |= p->transform().map(stroke).boundingRect();
        }

        p->translate(m_glyphs[unicode].m_horizAdvX, 0);
    }

    p->restore();
}

void QSvgFont::setFamilyName(const QString &name)
{
    m_familyName = name;
}

void QSvgFont::setUnitsPerEm(qreal upem)
{
    m_unitsPerEm = upem;
}

QT_END_NAMESPACE
