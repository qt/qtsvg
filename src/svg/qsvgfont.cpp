/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

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


void QSvgFont::draw(QPainter *p, const QPointF &point, const QString &str, qreal pixelSize, Qt::Alignment alignment) const
{
    p->save();
    p->translate(point);
    p->scale(pixelSize / m_unitsPerEm, -pixelSize / m_unitsPerEm);

    // Calculate the text width to be used for alignment
    int textWidth = 0;
    QString::const_iterator itr = str.constBegin();
    for ( ; itr != str.constEnd(); ++itr) {
        QChar unicode = *itr;
        if (!m_glyphs.contains(*itr)) {
            unicode = 0;
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
            unicode = 0;
            if (!m_glyphs.contains(unicode))
                continue;
        }
        p->drawPath(m_glyphs[unicode].m_path);
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
