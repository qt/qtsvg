// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgcsshandler_p.h"
#include <QtSvg/private/qsvganimatedproperty_p.h>
#include <QtSvg/private/qsvgutils_p.h>

QT_BEGIN_NAMESPACE

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
    if (m_cachedAnimations.contains(name))
        return m_cachedAnimations[name];

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
            }
        }
    }

    for (auto it = animatedProperies.begin(); it != animatedProperies.end(); it++)
        animation->appendProperty(it.value());
    m_cachedAnimations[name] = animation;
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
                QSvgUtils::LengthType type;
                qreal rotationAngle = QSvgUtils::parseLength(args[0], &type);
                property->appendRotation(rotationAngle);
                property->appendCenterOfRotation(QPointF(0, 0));
            } else if (transformType == QStringLiteral("skew")) {
                QSvgUtils::LengthType type;
                qreal skew0 = QSvgUtils::parseLength(args[0], &type);
                skew0 = QSvgUtils::convertToPixels(skew0, false, type);
                qreal skew1 = QSvgUtils::parseLength(args[1], &type);
                skew1 = QSvgUtils::convertToPixels(skew1, false, type);
                property->appendSkew(QPointF(skew0, skew1));
            } else if (transformType == QStringLiteral("matrix")) {
                QSvgUtils::LengthType type;
                qreal translate0 = QSvgUtils::parseLength(args[4], &type);
                translate0 = QSvgUtils::convertToPixels(translate0, false, type);
                qreal translate1 = QSvgUtils::parseLength(args[5], &type);
                translate1 = QSvgUtils::convertToPixels(translate1, false, type);
                qreal scale0 = QSvgUtils::toDouble(args[0].trimmed());
                qreal scale1 = QSvgUtils::toDouble(args[3].trimmed());
                qreal skew0 = QSvgUtils::parseLength(args[1], &type);
                skew0 = QSvgUtils::convertToPixels(skew0, false, type);
                qreal skew1 = QSvgUtils::parseLength(args[2], &type);
                skew1 = QSvgUtils::convertToPixels(skew1, false, type);
                property->appendSkew(QPointF(skew0, skew1));
                property->appendTranslation(QPointF(translate0, translate1));
                property->appendScale(QPointF(scale0, scale1));
            }
        }
    }
}

QT_END_NAMESPACE
