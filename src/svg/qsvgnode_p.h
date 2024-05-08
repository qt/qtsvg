// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGNODE_P_H
#define QSVGNODE_P_H

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

#include "qsvgstyle_p.h"
#include "qtsvgglobal_p.h"
#include "qsvghelper_p.h"

#include "QtCore/qstring.h"
#include "QtCore/qhash.h"

QT_BEGIN_NAMESPACE

class QPainter;
class QSvgTinyDocument;

class Q_SVG_PRIVATE_EXPORT QSvgNode
{
public:
    enum Type
    {
        Doc,
        Group,
        Defs,
        Switch,
        Animation,
        Circle,
        Ellipse,
        Image,
        Line,
        Path,
        Polygon,
        Polyline,
        Rect,
        Text,
        Textarea,
        Tspan,
        Use,
        Video,
        Mask,
        Symbol,
        Marker,
        Pattern,
        Filter,
        FeMerge,
        FeMergenode,
        FeColormatrix,
        FeGaussianblur,
        FeOffset,
        FeComposite,
        FeFlood,
        FeUnsupported
    };
    enum DisplayMode {
        InlineMode,
        BlockMode,
        ListItemMode,
        RunInMode,
        CompactMode,
        MarkerMode,
        TableMode,
        InlineTableMode,
        TableRowGroupMode,
        TableHeaderGroupMode,
        TableFooterGroupMode,
        TableRowMode,
        TableColumnGroupMode,
        TableColumnMode,
        TableCellMode,
        TableCaptionMode,
        NoneMode,
        InheritMode
    };

public:
    QSvgNode(QSvgNode *parent=0);
    virtual ~QSvgNode();
    void draw(QPainter *p, QSvgExtraStates &states);
    virtual bool separateFillStroke() const {return false;}
    virtual void drawCommand(QPainter *p, QSvgExtraStates &states) = 0;
    void fillThenStroke(QPainter *p, QSvgExtraStates &states);
    QImage drawIntoBuffer(QPainter *p, QSvgExtraStates &states, const QRect &boundsRect);
    void applyMaskToBuffer(QImage *proxy, QImage mask) const;
    void drawWithMask(QPainter *p, QSvgExtraStates &states, const QImage &mask, const QRect &boundsRect);
    void applyBufferToCanvas(QPainter *p, QImage proxy) const;

    QSvgNode *parent() const;
    bool isDescendantOf(const QSvgNode *parent) const;

    void appendStyleProperty(QSvgStyleProperty *prop, const QString &id);
    void applyStyle(QPainter *p, QSvgExtraStates &states) const;
    void applyStyleRecursive(QPainter *p, QSvgExtraStates &states) const;
    void revertStyle(QPainter *p, QSvgExtraStates &states) const;
    void revertStyleRecursive(QPainter *p, QSvgExtraStates &states) const;
    QSvgStyleProperty *styleProperty(QSvgStyleProperty::Type type) const;
    QSvgPaintStyleProperty *styleProperty(const QString &id) const;

    QSvgTinyDocument *document() const;

    virtual Type type() const = 0;
    QString typeName() const;
    virtual QRectF fastBounds(QPainter *p, QSvgExtraStates &states) const;
    virtual QRectF bounds(QPainter *p, QSvgExtraStates &states) const;
    virtual QRectF transformedBounds(QPainter *p, QSvgExtraStates &states) const;
    QRectF transformedBounds() const;

    void setRequiredFeatures(const QStringList &lst);
    const QStringList & requiredFeatures() const;

    void setRequiredExtensions(const QStringList &lst);
    const QStringList & requiredExtensions() const;

    void setRequiredLanguages(const QStringList &lst);
    const QStringList & requiredLanguages() const;

    void setRequiredFormats(const QStringList &lst);
    const QStringList & requiredFormats() const;

    void setRequiredFonts(const QStringList &lst);
    const QStringList & requiredFonts() const;

    void setVisible(bool visible);
    bool isVisible() const;

    void setDisplayMode(DisplayMode display);
    DisplayMode displayMode() const;

    QString nodeId() const;
    void setNodeId(const QString &i);

    QString xmlClass() const;
    void setXmlClass(const QString &str);

    QString maskId() const;
    void setMaskId(const QString &str);
    bool hasMask() const;

    QString filterId() const;
    void setFilterId(const QString &str);
    bool hasFilter() const;

    QString markerStartId() const;
    void setMarkerStartId(const QString &str);
    bool hasMarkerStart() const;

    QString markerMidId() const;
    void setMarkerMidId(const QString &str);
    bool hasMarkerMid() const;

    QString markerEndId() const;
    void setMarkerEndId(const QString &str);
    bool hasMarkerEnd() const;

    bool hasAnyMarker() const;

    virtual bool shouldDrawNode(QPainter *p, QSvgExtraStates &states) const;
    const QSvgStyle &style() const { return m_style; }
protected:
    mutable QSvgStyle m_style;

    static qreal strokeWidth(QPainter *p);
    static void initPainter(QPainter *p);

private:
    QSvgNode   *m_parent;

    QStringList m_requiredFeatures;
    QStringList m_requiredExtensions;
    QStringList m_requiredLanguages;
    QStringList m_requiredFormats;
    QStringList m_requiredFonts;

    bool        m_visible;

    QString m_id;
    QString m_class;
    QString m_maskId;
    QString m_filterId;
    QString m_markerStartId;
    QString m_markerMidId;
    QString m_markerEndId;


    DisplayMode m_displayMode;
    mutable QRectF m_cachedBounds;

    friend class QSvgTinyDocument;
};

inline QSvgNode *QSvgNode::parent() const
{
    return m_parent;
}

inline bool QSvgNode::isVisible() const
{
    return m_visible;
}

inline QString QSvgNode::nodeId() const
{
    return m_id;
}

inline QString QSvgNode::xmlClass() const
{
    return m_class;
}

QT_END_NAMESPACE

#endif // QSVGNODE_P_H
