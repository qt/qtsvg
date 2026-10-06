// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qsvgvisitor_p.h"
#include <QStack>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

bool isStructure(const QSvgNode *node)
{
    switch (node->type()) {
    case QSvgNode::Switch:
    case QSvgNode::Doc:
    case QSvgNode::Defs:
    case QSvgNode::Group:
    case QSvgNode::Mask:
    case QSvgNode::Symbol:
    case QSvgNode::Filter:
    case QSvgNode::FeMerge:
    case QSvgNode::FeMergenode:
    case QSvgNode::FeColormatrix:
    case QSvgNode::FeGaussianblur:
    case QSvgNode::FeOffset:
    case QSvgNode::FeComposite:
    case QSvgNode::FeFlood:
    case QSvgNode::FeBlend:
    case QSvgNode::FeUnsupported:
    case QSvgNode::Marker:
    case QSvgNode::Pattern:
        return true;
    case QSvgNode::AnimateColor:
    case QSvgNode::AnimateTransform:
    case QSvgNode::Circle:
    case QSvgNode::Ellipse:
    case QSvgNode::Image:
    case QSvgNode::Line:
    case QSvgNode::Path:
    case QSvgNode::Polygon:
    case QSvgNode::Polyline:
    case QSvgNode::Rect:
    case QSvgNode::Text:
    case QSvgNode::Textarea:
    case QSvgNode::Tspan:
    case QSvgNode::Use:
    case QSvgNode::Video:
    case QSvgNode::Font:
        return false;
    }

    Q_UNREACHABLE_RETURN(false);
}

} // namespace

QSvgVisitor::~QSvgVisitor()
    = default;

void QSvgVisitor::traverse(const QSvgNode *node)
{
    // A pair that keeps track of a node and a visited flag.
    using NodeState = std::pair<const QSvgNode *, bool>;
    QStack<NodeState> nodes;
    nodes.push({node, false});

    do {
        NodeState state = nodes.pop();
        const QSvgNode *current = state.first;
        const bool visited = state.second;
        if (isStructure(current)) {
            const QSvgStructureNode *structure = static_cast<const QSvgStructureNode *>(current);
            if (!visited) {
                if (!traverseStructureNodeStart(structure))
                    continue;
                nodes.push({structure, true});
                for (auto it = structure->renderers().crbegin(); it != structure->renderers().crend(); it++)
                    nodes.push({it->get(), false});
            } else {
                traverseStructureNodeEnd(structure);
            }
        } else {
            traverseLeafNode(current);
        }

    } while (!nodes.isEmpty());
}

bool QSvgVisitor::traverseStructureNodeStart(const QSvgStructureNode *node)
{
    switch (node->type()) {
    case QSvgNode::Switch:
        return visitSwitchNodeStart(static_cast<const QSvgSwitch *>(node));
    case QSvgNode::Doc:
        return visitDocumentNodeStart(static_cast<const QSvgDocument *>(node));
    case QSvgNode::Defs:
        return visitDefsNodeStart(static_cast<const QSvgDefs *>(node));
    case QSvgNode::Group:
        return visitGroupNodeStart(static_cast<const QSvgG *>(node));
    case QSvgNode::Mask:
        return visitMaskNodeStart(static_cast<const QSvgMask *>(node));
    case QSvgNode::Symbol:
        return visitSymbolNodeStart(static_cast<const QSvgSymbol *>(node));
    case QSvgNode::Filter:
        return visitFilterNodeStart(static_cast<const QSvgFilterContainer *>(node));
    case QSvgNode::Marker:
        return visitMarkerNodeStart(static_cast<const QSvgMarker *>(node));
    case QSvgNode::Pattern:
        return visitPatternNodeStart(static_cast<const QSvgPattern *>(node));
    case QSvgNode::FeMerge:
    case QSvgNode::FeMergenode:
    case QSvgNode::FeColormatrix:
    case QSvgNode::FeGaussianblur:
    case QSvgNode::FeOffset:
    case QSvgNode::FeComposite:
    case QSvgNode::FeFlood:
    case QSvgNode::FeBlend:
        return visitFeFilterPrimitiveNodeStart(static_cast<const QSvgFeFilterPrimitive *>(node));
    // Enum values that are either not supported or should not be visited:
    case QSvgNode::FeUnsupported:
        qDebug() << "Unhandled type in switch" << node->type();
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return true;
}

void QSvgVisitor::traverseStructureNodeEnd(const QSvgStructureNode *node)
{
    switch (node->type()) {
    case QSvgNode::Switch:
        visitSwitchNodeEnd(static_cast<const QSvgSwitch *>(node));
        break;
    case QSvgNode::Doc:
        visitDocumentNodeEnd(static_cast<const QSvgDocument *>(node));
        break;
    case QSvgNode::Defs:
        visitDefsNodeEnd(static_cast<const QSvgDefs *>(node));
        break;
    case QSvgNode::Group:
        visitGroupNodeEnd(static_cast<const QSvgG *>(node));
        break;
    case QSvgNode::Mask:
        visitMaskNodeEnd(static_cast<const QSvgMask *>(node));
        break;
    case QSvgNode::Symbol:
        visitSymbolNodeEnd(static_cast<const QSvgSymbol *>(node));
        break;
    case QSvgNode::Filter:
        visitFilterNodeEnd(static_cast<const QSvgFilterContainer *>(node));
        break;
    case QSvgNode::Marker:
        visitMarkerNodeEnd(static_cast<const QSvgMarker *>(node));
        break;
    case QSvgNode::Pattern:
        visitPatternNodeEnd(static_cast<const QSvgPattern *>(node));
        break;
    case QSvgNode::FeMerge:
    case QSvgNode::FeMergenode:
    case QSvgNode::FeColormatrix:
    case QSvgNode::FeGaussianblur:
    case QSvgNode::FeOffset:
    case QSvgNode::FeComposite:
    case QSvgNode::FeFlood:
    case QSvgNode::FeBlend:
        visitFeFilterPrimitiveNodeEnd(static_cast<const QSvgFeFilterPrimitive *>(node));
        break;
    // Enum values that are either not supported or should not be visited:
    case QSvgNode::FeUnsupported:
        qDebug() << "Unhandled type in switch" << node->type();
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

void QSvgVisitor::traverseLeafNode(const QSvgNode *node)
{
    switch (node->type()) {
    case QSvgNode::AnimateColor:
    case QSvgNode::AnimateTransform:
        visitAnimateNode(static_cast<const QSvgAnimateNode *>(node));
        break;
    case QSvgNode::Circle:
    case QSvgNode::Ellipse:
        visitEllipseNode(static_cast<const QSvgEllipse *>(node));
        break;
    case QSvgNode::Image:
        visitImageNode(static_cast<const QSvgImage *>(node));
        break;
    case QSvgNode::Line:
        visitLineNode(static_cast<const QSvgLine *>(node));
        break;
    case QSvgNode::Path:
        visitPathNode(static_cast<const QSvgPath *>(node));
        break;
    case QSvgNode::Polygon:
        visitPolygonNode(static_cast<const QSvgPolygon *>(node));
        break;
    case QSvgNode::Polyline:
        visitPolylineNode(static_cast<const QSvgPolyline *>(node));
        break;
    case QSvgNode::Rect:
        visitRectNode(static_cast<const QSvgRect *>(node));
        break;
    case QSvgNode::Text:
    case QSvgNode::Textarea:
        visitTextNode(static_cast<const QSvgText *>(node));
        break;
    case QSvgNode::Tspan:
        visitTspanNode(static_cast<const QSvgTspan *>(node));
        break;
    case QSvgNode::Use:
        visitUseNode(static_cast<const QSvgUse *>(node));
        break;
    case QSvgNode::Video:
        visitVideoNode(static_cast<const QSvgVideo *>(node));
        break;
    // Enum values that are either not supported or should not be visited:
    case QSvgNode::Font:
        qDebug() << "Unhandled type in switch" << node->type();
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

QT_END_NAMESPACE
