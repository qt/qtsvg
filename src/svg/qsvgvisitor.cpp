// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgvisitor_p.h"

QT_BEGIN_NAMESPACE

void QSvgVisitor::traverse(const QSvgStructureNode *node)
{
    switch (node->type()) {
    case QSvgNode::SWITCH:
        if (!visitSwitchNodeStart(static_cast<const QSvgSwitch *>(node)))
            return;
        break;
    case QSvgNode::DOC:
        if (!visitDocumentNodeStart(static_cast<const QSvgTinyDocument *>(node)))
            return;
        break;
    case QSvgNode::DEFS:
        if (!visitDefsNodeStart(static_cast<const QSvgDefs *>(node)))
            return;
        break;
    case QSvgNode::G:
        if (!visitGroupNodeStart(static_cast<const QSvgG *>(node)))
            return;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    for (const auto *child : node->renderers()) {
        switch (child->type()) {
        case QSvgNode::SWITCH:
        case QSvgNode::DOC:
        case QSvgNode::DEFS:
        case QSvgNode::G:
            traverse(static_cast<const QSvgStructureNode *>(child));
            break;
        case QSvgNode::ANIMATION:
            visitAnimationNode(static_cast<const QSvgAnimation *>(child));
            break;
        case QSvgNode::CIRCLE:
        case QSvgNode::ELLIPSE:
            visitEllipseNode(static_cast<const QSvgEllipse *>(child));
            break;
        case QSvgNode::IMAGE:
            visitImageNode(static_cast<const QSvgImage *>(child));
            break;
        case QSvgNode::LINE:
            visitLineNode(static_cast<const QSvgLine *>(child));
            break;
        case QSvgNode::PATH:
            visitPathNode(static_cast<const QSvgPath *>(child));
            break;
        case QSvgNode::POLYGON:
            visitPolygonNode(static_cast<const QSvgPolygon *>(child));
            break;
        case QSvgNode::POLYLINE:
            visitPolylineNode(static_cast<const QSvgPolyline *>(child));
            break;
        case QSvgNode::RECT:
            visitRectNode(static_cast<const QSvgRect *>(child));
            break;
        case QSvgNode::TEXT:
        case QSvgNode::TEXTAREA:
            visitTextNode(static_cast<const QSvgText *>(child));
            break;
        case QSvgNode::TSPAN:
            visitTspanNode(static_cast<const QSvgTspan *>(child));
            break;
        case QSvgNode::USE:
            visitUseNode(static_cast<const QSvgUse *>(child));
            break;
        case QSvgNode::VIDEO:
            visitVideoNode(static_cast<const QSvgVideo *>(child));
            break;

        // Enum values that don't have any QSvgNode classes yet:
           case QSvgNode::MASK:
           case QSvgNode::SYMBOL:
           case QSvgNode::MARKER:
           case QSvgNode::PATTERN:
           case QSvgNode::FILTER:
           case QSvgNode::FEMERGE:
           case QSvgNode::FEMERGENODE:
           case QSvgNode::FECOLORMATRIX:
           case QSvgNode::FEGAUSSIANBLUR:
           case QSvgNode::FEOFFSET:
           case QSvgNode::FECOMPOSITE:
           case QSvgNode::FEFLOOD:
            qDebug() << "Unhandled type in switch" << child->type();
            break;

           case QSvgNode::ARC: // Not used: to be removed
            Q_UNREACHABLE();
            break;
        }
    }
    switch (node->type()) {
    case QSvgNode::SWITCH:
        visitSwitchNodeEnd(static_cast<const QSvgSwitch *>(node));
        break;
    case QSvgNode::DOC:
        visitDocumentNodeEnd(static_cast<const QSvgTinyDocument *>(node));
        break;
    case QSvgNode::DEFS:
        visitDefsNodeEnd(static_cast<const QSvgDefs *>(node));
        break;
    case QSvgNode::G:
        visitGroupNodeEnd(static_cast<const QSvgG *>(node));
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

QT_END_NAMESPACE
