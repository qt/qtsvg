// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgutils_p.h"
#include <cmath>

QT_BEGIN_NAMESPACE

namespace QSvgUtils {

// '0' is 0x30 and '9' is 0x39
bool isDigit(ushort ch)
{
    static quint16 magic = 0x3ff;
    return ((ch >> 4) == 3) && (magic >> (ch & 15));
}

qreal toDouble(const QChar *&str)
{
    const int maxLen = 255;//technically doubles can go til 308+ but whatever
    char temp[maxLen+1];
    int pos = 0;

    if (*str == QLatin1Char('-')) {
        temp[pos++] = '-';
        ++str;
    } else if (*str == QLatin1Char('+')) {
        ++str;
    }
    while (isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    if (*str == QLatin1Char('.') && pos < maxLen) {
        temp[pos++] = '.';
        ++str;
    }
    while (isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    bool exponent = false;
    if ((*str == QLatin1Char('e') || *str == QLatin1Char('E')) && pos < maxLen) {
        exponent = true;
        temp[pos++] = 'e';
        ++str;
        if ((*str == QLatin1Char('-') || *str == QLatin1Char('+')) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
        while (isDigit(str->unicode()) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
    }

    temp[pos] = '\0';

    qreal val;
    if (!exponent && pos < 10) {
        int ival = 0;
        const char *t = temp;
        bool neg = false;
        if (*t == '-') {
            neg = true;
            ++t;
        }
        while (*t && *t != '.') {
            ival *= 10;
            ival += (*t) - '0';
            ++t;
        }
        if (*t == '.') {
            ++t;
            int div = 1;
            while (*t) {
                ival *= 10;
                ival += (*t) - '0';
                div *= 10;
                ++t;
            }
            val = ((qreal)ival)/((qreal)div);
        } else {
            val = ival;
        }
        if (neg)
            val = -val;
    } else {
        val = QByteArray::fromRawData(temp, pos).toDouble();
        // Do not tolerate values too wild to be represented normally by floats
        if (qFpClassify(float(val)) != FP_NORMAL)
            val = 0;
    }
    return val;

}

qreal toDouble(QStringView str, bool *ok)
{
    const QChar *c = str.constData();
    qreal res = (c == nullptr ? qreal{} : toDouble(c));
    if (ok)
        *ok = (c == (str.constData() + str.size()));
    return res;
}

qreal parseLength(QStringView str, LengthType *type, bool *ok)
{
    QStringView numStr = str.trimmed();

    if (numStr.isEmpty()) {
        if (ok)
            *ok = false;
        *type = LengthType::LT_OTHER;
        return false;
    }
    if (numStr.endsWith(QLatin1Char('%'))) {
        numStr.chop(1);
        *type = LengthType::LT_PERCENT;
    } else if (numStr.endsWith(QLatin1String("px"))) {
        numStr.chop(2);
        *type = LengthType::LT_PX;
    } else if (numStr.endsWith(QLatin1String("pc"))) {
        numStr.chop(2);
        *type = LengthType::LT_PC;
    } else if (numStr.endsWith(QLatin1String("pt"))) {
        numStr.chop(2);
        *type = LengthType::LT_PT;
    } else if (numStr.endsWith(QLatin1String("mm"))) {
        numStr.chop(2);
        *type = LengthType::LT_MM;
    } else if (numStr.endsWith(QLatin1String("cm"))) {
        numStr.chop(2);
        *type = LengthType::LT_CM;
    } else if (numStr.endsWith(QLatin1String("in"))) {
        numStr.chop(2);
        *type = LengthType::LT_IN;
    } else {
        // default coordinate system
        *type = LengthType::LT_PX;
    }
    qreal len = toDouble(numStr, ok);
    return len;
}

// this should really be called convertToDefaultCoordinateSystem
// and convert when type != QSvgHandler::defaultCoordinateSystem
qreal convertToPixels(qreal len, bool , LengthType type)
{

    switch (type) {
    case LengthType::LT_PERCENT:
        break;
    case LengthType::LT_PX:
        break;
    case LengthType::LT_PC:
        break;
    case LengthType::LT_PT:
        return len * 1.25;
        break;
    case LengthType::LT_MM:
        return len * 3.543307;
        break;
    case LengthType::LT_CM:
        return len * 35.43307;
        break;
    case LengthType::LT_IN:
        return len * 90;
        break;
    case LengthType::LT_OTHER:
        break;
    default:
        break;
    }
    return len;
}

};

QT_END_NAMESPACE

