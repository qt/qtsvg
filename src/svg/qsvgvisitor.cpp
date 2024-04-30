// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgvisitor_p.h"

QT_BEGIN_NAMESPACE

void QSvgVisitor::traverse(const QSvgStructureNode *node)
{
    switch (node->type()) {
    case QSvgNode::Switch:
        if (!visitSwitchNodeStart(static_cast<const QSvgSwitch *>(node)))
            return;
        break;
    case QSvgNode::Doc:
        if (!visitDocumentNodeStart(static_cast<const QSvgTinyDocument *>(node)))
            return;
        break;
    case QSvgNode::Defs:
        if (!visitDefsNodeStart(static_cast<const QSvgDefs *>(node)))
            return;
        break;
    case QSvgNode::Group:
        if (!visitGroupNodeStart(static_cast<const QSvgG *>(node)))
            return;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    for (const auto *child : node->renderers()) {
        switch (child->type()) {
        case QSvgNode::Switch:
        case QSvgNode::Doc:
        case QSvgNode::Defs:
        case QSvgNode::Group:
            traverse(static_cast<const QSvgStructureNode *>(child));
            break;
        case QSvgNode::Animation:
            visitAnimationNode(static_cast<const QSvgAnimation *>(child));
            break;
        case QSvgNode::Circle:
        case QSvgNode::Ellipse:
            visitEllipseNode(static_cast<const QSvgEllipse *>(child));
            break;
        case QSvgNode::Image:
            visitImageNode(static_cast<const QSvgImage *>(child));
            break;
        case QSvgNode::Line:
            visitLineNode(static_cast<const QSvgLine *>(child));
            break;
        case QSvgNode::Path:
            visitPathNode(static_cast<const QSvgPath *>(child));
            break;
        case QSvgNode::Polygon:
            visitPolygonNode(static_cast<const QSvgPolygon *>(child));
            break;
        case QSvgNode::Polyline:
            visitPolylineNode(static_cast<const QSvgPolyline *>(child));
            break;
        case QSvgNode::Rect:
            visitRectNode(static_cast<const QSvgRect *>(child));
            break;
        case QSvgNode::Text:
        case QSvgNode::Textarea:
            visitTextNode(static_cast<const QSvgText *>(child));
            break;
        case QSvgNode::Tspan:
            visitTspanNode(static_cast<const QSvgTspan *>(child));
            break;
        case QSvgNode::Use:
            visitUseNode(static_cast<const QSvgUse *>(child));
            break;
        case QSvgNode::Video:
            visitVideoNode(static_cast<const QSvgVideo *>(child));
            break;

        // Enum values that don't have any QSvgNode classes yet:
           case QSvgNode::Mask:
           case QSvgNode::Symbol:
           case QSvgNode::Marker:
           case QSvgNode::Pattern:
           case QSvgNode::Filter:
           case QSvgNode::FeMerge:
           case QSvgNode::FeMergenode:
           case QSvgNode::FeColormatrix:
           case QSvgNode::FeGaussianblur:
           case QSvgNode::FeOffset:
           case QSvgNode::FeComposite:
           case QSvgNode::FeFlood:
           case QSvgNode::FeUnsupported:
            qDebug() << "Unhandled type in switch" << child->type();
            break;
        }
    }
    switch (node->type()) {
    case QSvgNode::Switch:
        visitSwitchNodeEnd(static_cast<const QSvgSwitch *>(node));
        break;
    case QSvgNode::Doc:
        visitDocumentNodeEnd(static_cast<const QSvgTinyDocument *>(node));
        break;
    case QSvgNode::Defs:
        visitDefsNodeEnd(static_cast<const QSvgDefs *>(node));
        break;
    case QSvgNode::Group:
        visitGroupNodeEnd(static_cast<const QSvgG *>(node));
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

QT_END_NAMESPACE
