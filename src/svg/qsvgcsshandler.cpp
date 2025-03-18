// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qsvgcsshandler_p.h"
#include <QtSvg/private/qsvganimatedproperty_p.h>
#include <QtSvg/private/qsvgutils_p.h>
#include <QtGui/private/qmath_p.h>

QT_BEGIN_NAMESPACE

// Parses the angle from a string and convert it to degrees.
static qreal qsvg_parseAngle(QStringView str, bool *ok = nullptr)
{
    QStringView numStr = str.trimmed();

    if (numStr.isEmpty()) {
        if (ok)
            *ok = false;
        return false;
    }

    qreal unitFactor;
    if (numStr.endsWith(QLatin1String("deg"))) {
        numStr.chop(3);
        unitFactor = 1.0;
    } else if (numStr.endsWith(QLatin1String("grad"))) {
        numStr.chop(4);
        // deg = grad * 0.9;
        unitFactor = 0.9;
    } else if (numStr.endsWith(QLatin1String("rad"))) {
        numStr.chop(3);
        unitFactor = 180.0 / Q_PI;
    } else if (numStr.endsWith(QLatin1String("turn"))) {
        numStr.chop(4);
        // one circle = one turn
        unitFactor = 360.0;
    } else {
        unitFactor = 0.0;
    }

    return QSvgUtils::toDouble(numStr, ok) * unitFactor;
}

void QSvgCssHandler::collectAnimations(const QCss::StyleSheet &sheet)
{
    auto sortFunction = [](QCss::AnimationRule::AnimationRuleSet r1, QCss::AnimationRule::AnimationRuleSet r2) {
        return r1.keyFrame < r2.keyFrame;
    };

    QList<QCss::AnimationRule> animationRules = sheet.animationRules;
    for (QCss::AnimationRule rule : animationRules) {
        std::sort(rule.ruleSets.begin(), rule.ruleSets.end(), sortFunction);
        m_animations[rule.animName] = rule;
    }
}

QSvgCssAnimation *QSvgCssHandler::createAnimation(const QString &name)
{
    if (!m_animations.contains(name))
        return nullptr;

    QCss::AnimationRule animationRule = m_animations[name];
    QHash<QString, QSvgAbstractAnimatedProperty*> animatedProperies;
    QSvgCssAnimation *animation = new QSvgCssAnimation;

    for (const auto &ruleSet : std::as_const(animationRule.ruleSets)) {
        for (QCss::Declaration decl : ruleSet.declarations) {
            if (decl.d->property == QStringLiteral("fill") || decl.d->property == QStringLiteral("stroke")) {
                QSvgAnimatedPropertyColor *prop = nullptr;
                if (!animatedProperies.contains(decl.d->property))
                    animatedProperies[decl.d->property] = QSvgAbstractAnimatedProperty::createAnimatedProperty(decl.d->property);
                prop = static_cast<QSvgAnimatedPropertyColor *>(animatedProperies[decl.d->property]);
                prop->appendKeyFrame(ruleSet.keyFrame);
                updateColorProperty(decl, prop);
            } else if (decl.d->property == QStringLiteral("transform")) {
                QSvgAnimatedPropertyTransform *prop = nullptr;
                if (!animatedProperies.contains(decl.d->property))
                    animatedProperies[decl.d->property] = QSvgAbstractAnimatedProperty::createAnimatedProperty(decl.d->property);
                prop = static_cast<QSvgAnimatedPropertyTransform *>(animatedProperies[decl.d->property]);
                prop->appendKeyFrame(ruleSet.keyFrame);
                updateTransformProperty(decl, prop);
            } else if (decl.d->property == QStringLiteral("fill-opacity") || decl.d->property == QStringLiteral("stroke-opacity")
                       || decl.d->property == QStringLiteral("opacity")) {
                QSvgAnimatedPropertyFloat *prop = nullptr;
                if (!animatedProperies.contains(decl.d->property))
                    animatedProperies[decl.d->property] = QSvgAbstractAnimatedProperty::createAnimatedProperty(decl.d->property);
                prop = static_cast<QSvgAnimatedPropertyFloat *>(animatedProperies[decl.d->property]);
                prop->appendKeyFrame(ruleSet.keyFrame);
                QString opacity = decl.d->values.first().toString();
                prop->appendValue(opacity.toDouble());
            }
        }
    }

    for (auto it = animatedProperies.begin(); it != animatedProperies.end(); it++)
        animation->appendProperty(it.value());

    return animation;
}

void QSvgCssHandler::updateColorProperty(const QCss::Declaration &decl, QSvgAnimatedPropertyColor *property)
{
    QString colorStr = decl.d->values.first().toString();
    QColor color = QColor::fromString(colorStr);
    property->appendColor(color);
}

void QSvgCssHandler::updateTransformProperty(const QCss::Declaration &decl, QSvgAnimatedPropertyTransform *property)
{
    for (QCss::Value val : decl.d->values) {
        if (val.type == QCss::Value::Function) {
            QStringList lst = val.variant.toStringList();
            QStringView transformType = lst[0];
            QStringList args = lst[1].split(QStringLiteral(","), Qt::SkipEmptyParts);
            if (transformType == QStringLiteral("scale")) {
                qreal scale0 = QSvgUtils::toDouble(args[0].trimmed());
                qreal scale1 = QSvgUtils::toDouble(args[1].trimmed());
                property->appendScale(QPointF(scale0, scale1));
            } else if (transformType == QStringLiteral("translate")) {
                QSvgUtils::LengthType type;
                qreal translate0 = QSvgUtils::parseLength(args[0], &type);
                translate0 = QSvgUtils::convertToPixels(translate0, false, type);
                qreal translate1 = QSvgUtils::parseLength(args[1], &type);
                translate1 = QSvgUtils::convertToPixels(translate1, false, type);
                property->appendTranslation(QPointF(translate0, translate1));
            } else if (transformType == QStringLiteral("rotate")) {
                qreal rotationAngle = qsvg_parseAngle(args[0]);
                property->appendRotation(rotationAngle);
                property->appendCenterOfRotation(QPointF(0, 0));
            } else if (transformType == QStringLiteral("skew")) {
                qreal skew0 = qsvg_parseAngle(args[0]);
                qreal skew1 = qsvg_parseAngle(args[1]);
                property->appendSkew(QPointF(skew0, skew1));
            } else if (transformType == QStringLiteral("matrix")) {
                QSvgUtils::LengthType type;
                qreal translate0 = QSvgUtils::parseLength(args[4], &type);
                translate0 = QSvgUtils::convertToPixels(translate0, false, type);
                qreal translate1 = QSvgUtils::parseLength(args[5], &type);
                translate1 = QSvgUtils::convertToPixels(translate1, false, type);
                qreal scale0 = QSvgUtils::toDouble(args[0].trimmed());
                qreal scale1 = QSvgUtils::toDouble(args[3].trimmed());
                qreal skew0 = QSvgUtils::toDouble((args[1].trimmed()));
                qreal skew1 = QSvgUtils::toDouble((args[2].trimmed()));
                property->appendSkew(QPointF(skew0, skew1));
                property->appendTranslation(QPointF(translate0, translate1));
                property->appendScale(QPointF(scale0, scale1));
            }
        }
    }
}

QT_END_NAMESPACE
