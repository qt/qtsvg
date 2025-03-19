// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qsvgcsshandler_p.h"
#include <QtSvg/private/qsvgstyleselector_p.h>
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

QSvgCssHandler::QSvgCssHandler()
    : m_selector(new QSvgStyleSelector)
{

}

QSvgCssHandler::~QSvgCssHandler()
{
    delete m_selector;
    m_selector = nullptr;
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

void QSvgCssHandler::parseStyleSheet(const QStringView str)
{
    QString css = str.toString();
    QCss::StyleSheet sheet;
    QCss::Parser(css).parse(&sheet);
    m_selector->styleSheets.append(sheet);

    collectAnimations(sheet);
}

void QSvgCssHandler::parseCSStoXMLAttrs(const QList<QCss::Declaration> &declarations, QXmlStreamAttributes &attributes) const
{
    for (int i = 0; i < declarations.size(); ++i) {
        const QCss::Declaration &decl = declarations.at(i);
        if (decl.d->property.isEmpty())
            continue;
        QCss::Value val = decl.d->values.first();
        QString valueStr;
        const int valCount = decl.d->values.size();
        if (valCount != 1) {
            for (int i = 0; i < valCount; ++i) {
                valueStr += decl.d->values[i].toString();
                if (i + 1 < valCount)
                    valueStr += QLatin1Char(',');
            }
        } else {
            valueStr = val.toString();
        }
        if (val.type == QCss::Value::Uri) {
            valueStr.prepend(QLatin1String("url("));
            valueStr.append(QLatin1Char(')'));
        } else if (val.type == QCss::Value::Function) {
            QStringList lst = val.variant.toStringList();
            valueStr.append(lst.at(0));
            valueStr.append(QLatin1Char('('));
            for (int i = 1; i < lst.size(); ++i) {
                valueStr.append(lst.at(i));
                if ((i +1) < lst.size())
                    valueStr.append(QLatin1Char(','));
            }
            valueStr.append(QLatin1Char(')'));
        } else if (val.type == QCss::Value::KnownIdentifier) {
            switch (val.variant.toInt()) {
            case QCss::Value_None:
                valueStr = QLatin1String("none");
                break;
            default:
                break;
            }
        }

        attributes.append(QString(), decl.d->property, valueStr);
    }
}

void QSvgCssHandler::parseCSStoXMLAttrs(const QString &css, QXmlStreamAttributes &attributes) const
{
    // preprocess (for unicode escapes), tokenize and remove comments
    QCss::Parser parser(css);
    QString key;

    while (parser.hasNext()) {
        parser.skipSpace();

        if (!parser.hasNext())
            break;
        parser.next();

        QString name;
        QString value;

        if (parser.hasEscapeSequences) {
            key = parser.lexem();
            name = key;
        } else {
            const QCss::Symbol &sym = parser.symbol();
            name = sym.text.mid(sym.start, sym.len);
        }

        parser.skipSpace();
        if (!parser.test(QCss::COLON))
            break;

        parser.skipSpace();
        if (!parser.hasNext())
            break;

        const int firstSymbol = parser.index;
        int symbolCount = 0;
        do {
            parser.next();
            ++symbolCount;
        } while (parser.hasNext() && !parser.test(QCss::SEMICOLON));

        bool canExtractValueByRef = !parser.hasEscapeSequences;
        if (canExtractValueByRef) {
            int len = parser.symbols.at(firstSymbol).len;
            for (int i = firstSymbol + 1; i < firstSymbol + symbolCount; ++i) {
                len += parser.symbols.at(i).len;

                if (parser.symbols.at(i - 1).start + parser.symbols.at(i - 1).len
                    != parser.symbols.at(i).start) {
                    canExtractValueByRef = false;
                    break;
                }
            }
            if (canExtractValueByRef) {
                const QCss::Symbol &sym = parser.symbols.at(firstSymbol);
                value = sym.text.mid(sym.start, len);
            }
        }
        if (!canExtractValueByRef) {

            for (int i = firstSymbol; i < parser.index - 1; ++i)
                value += parser.symbols.at(i).lexem();
        }

        attributes.append(QString(), name, value);

        parser.skipSpace();
    }
}

void QSvgCssHandler::styleLookup(QSvgNode *node, QXmlStreamAttributes &attributes) const
{
    QCss::StyleSelector::NodePtr cssNode;
    cssNode.ptr = node;
    QList<QCss::Declaration> decls = m_selector->declarationsForNode(cssNode);

    parseCSStoXMLAttrs(decls, attributes);
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
