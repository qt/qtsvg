// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qsvgcssproperties_p.h"

QT_BEGIN_NAMESPACE

static int parseCssClockValue(QStringView str, bool *ok)
{
    int res = 0;
    int ms = 1000;
    str = str.trimmed();
    if (str.endsWith(QLatin1StringView("ms"))) {
        str.chop(2);
        ms = 1;
    } else if (str.endsWith(QLatin1StringView("s"))) {
        str.chop(1);
    } else {
        if (ok)
            *ok = false;
        return res;
    }
    double val = ms * str.toDouble(ok);
    if (ok) {
        if (val > std::numeric_limits<int>::min() && val < std::numeric_limits<int>::max())
            res = static_cast<int>(val);
        else
            *ok = false;
    }
    return res;
}

QSvgCssAnimationProperties::QSvgCssAnimationProperties(const QXmlStreamAttributes &attributes)
{
    QRegularExpression re(QLatin1StringView(";| "));
    for (int i = 0; i < attributes.size(); ++i) {
        const QXmlStreamAttribute &attribute = attributes.at(i);
        QStringView name = attribute.qualifiedName();
        if (name.isEmpty())
            continue;
        QStringView value = attribute.value();

        switch (name.at(0).unicode()) {

        case 'a':
            if (name == QLatin1StringView("animation-name"))
                m_names = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-duration"))
                m_durations = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-delay"))
                m_delays = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-iteration-count"))
                m_iterationCounts = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-direction"))
                m_directions = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-timing-function"))
                m_timingFunctions = value.split(re, Qt::SkipEmptyParts);
            if (name == QLatin1StringView("animation-fill-mode"))
                m_fillModes = value.split(re, Qt::SkipEmptyParts);
            break;

        default:
            break;
        }
    }
}

QList<QSvgAnimationProperty> QSvgCssAnimationProperties::parse() const
{
    bool ok;
    QList<QSvgAnimationProperty> parsedProperties;
    for (int i = 0; i < m_names.size(); i++) {
        QSvgAnimationProperty property;
        property.name = m_names.at(i);

        if (!m_durations.isEmpty()) {
            QStringView durationStr = m_durations.at(i % m_durations.size());
            int duration = parseCssClockValue(durationStr, &ok);
            property.duration = ok ? duration : 0;
        }

        if (!m_delays.isEmpty()) {
            QStringView delayStr = m_delays.at(i % m_delays.size());
            int delay = parseCssClockValue(delayStr, &ok);
            property.delay = ok ? delay : 0;
        }

        if (!m_iterationCounts.isEmpty()) {
            QStringView iterationStr = m_iterationCounts.at(i % m_iterationCounts.size());
            int iteration;
            if (iterationStr == QLatin1StringView("infinite")) {
                iteration = -1;
            } else {
                qreal count = iterationStr.toDouble(&ok);
                iteration = ok ? qMax(1.0, count) : 0;
            }
            property.iteration = iteration;
        }

        parsedProperties.append(property);
    }

    return parsedProperties;
}

QT_END_NAMESPACE
