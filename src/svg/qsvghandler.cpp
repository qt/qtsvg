// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qplatformdefs.h"

#include "qsvghandler_p.h"

#include "qsvgdocument_p.h"
#include "qsvgstructure_p.h"
#include "qsvgfilter_p.h"
#include "qsvgnode_p.h"
#include "qsvganimate_p.h"

#include "qpen.h"
#include "qpainterpath.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qtextformat.h"

#include <QtCore/private/qdataurl_p.h>
#include "qlist.h"
#include "qfileinfo.h"
#include "qfile.h"
#include "qdir.h"
#include "qdebug.h"
#include "qmath.h"
#include "qnumeric.h"
#include <qregularexpression.h>
#include "qtransform.h"
#include "qvarlengtharray.h"
#include "qimagereader.h"

#include "float.h"

#include <algorithm>
#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcSvgHandler, "qt.svg")

namespace {
namespace tokens {
// common
constexpr auto inherit = "inherit"_L1;
constexpr auto normal = "normal"_L1;
// font-style
constexpr auto italic = "italic"_L1;
constexpr auto oblique = "oblique"_L1;
// font-weight
constexpr auto bold = "bold"_L1;
constexpr auto bolder = "bolder"_L1;
constexpr auto lighter = "lighter"_L1;
// font-variant
constexpr auto small_caps = "small-caps"_L1;
// text-anchor
constexpr auto start = "start"_L1;
constexpr auto middle = "middle"_L1;
constexpr auto end = "end"_L1;
// comp-op
namespace compOp{
constexpr auto clear = "clear"_L1;
constexpr auto src = "src"_L1;
constexpr auto dst = "dst"_L1;
constexpr auto srcOver = "src-over"_L1;
constexpr auto dstOver = "dst-over"_L1;
constexpr auto srcIn = "src-in"_L1;
constexpr auto dstIn = "dst-in"_L1;
constexpr auto srcOut = "src-out"_L1;
constexpr auto dstOut = "dst-out"_L1;
constexpr auto srcAtop = "src-atop"_L1;
constexpr auto dstAtop = "dst-atop"_L1;
constexpr auto xorOp = "xor"_L1;
constexpr auto plus = "plus"_L1;
constexpr auto multiply = "multiply"_L1;
constexpr auto screen = "screen"_L1;
constexpr auto overlay = "overlay"_L1;
constexpr auto darken = "darken"_L1;
constexpr auto lighten = "lighten"_L1;
constexpr auto colorDodge = "color-dodge"_L1;
constexpr auto colorBurn = "color-burn"_L1;
constexpr auto hardLight = "hard-light"_L1;
constexpr auto softLight = "soft-light"_L1;
constexpr auto difference = "difference"_L1;
constexpr auto exclusion = "exclusion"_L1;
} // namespace compOp
} // namespace tokens
} // unnamed namespace

static QByteArray prefixMessage(const QByteArray &msg, const QXmlStreamReader *r)
{
    QByteArray result;
    if (r) {
        if (const QFile *file = qobject_cast<const QFile *>(r->device()))
            result.append(QFile::encodeName(QDir::toNativeSeparators(file->fileName())));
        else
            result.append(QByteArrayLiteral("<input>"));
        result.append(':');
        result.append(QByteArray::number(r->lineNumber()));
        if (const qint64 column = r->columnNumber()) {
            result.append(':');
            result.append(QByteArray::number(column));
        }
        result.append(QByteArrayLiteral(": "));
    }
    result.append(msg);
    return result;
}

static inline QByteArray msgProblemParsing(QStringView localName, const QXmlStreamReader *r)
{
    return prefixMessage("Problem parsing " + localName.toLocal8Bit(), r);
}

static inline QByteArray msgCouldNotResolveProperty(QStringView id, const QXmlStreamReader *r)
{
    return prefixMessage("Could not resolve property: " + id.toLocal8Bit(), r);
}

static QList<QStringView> splitWithDelimiter(QStringView delimitedList)
{
    static const QRegularExpression delimiterRE(QStringLiteral("[,\\s]+"));
    return delimitedList.split(delimiterRE, Qt::SkipEmptyParts);
}

// ======== duplicated from qcolor_p

static inline int qsvg_h2i(char hex, bool *ok = nullptr)
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';
    if (hex >= 'a' && hex <= 'f')
        return hex - 'a' + 10;
    if (hex >= 'A' && hex <= 'F')
        return hex - 'A' + 10;
    if (ok)
        *ok = false;
    return -1;
}

static inline int qsvg_hex2int(const char *s, bool *ok = nullptr)
{
    return (qsvg_h2i(s[0], ok) * 16) | qsvg_h2i(s[1], ok);
}

static inline int qsvg_hex2int(char s, bool *ok = nullptr)
{
    int h = qsvg_h2i(s, ok);
    return (h * 16) | h;
}

bool qsvg_get_hex_rgb(const char *name, QRgb *rgb)
{
    if(name[0] != '#')
        return false;
    name++;
    const size_t len = qstrlen(name);
    int r, g, b;
    bool ok = true;
    if (len == 12) {
        r = qsvg_hex2int(name, &ok);
        g = qsvg_hex2int(name + 4, &ok);
        b = qsvg_hex2int(name + 8, &ok);
    } else if (len == 9) {
        r = qsvg_hex2int(name, &ok);
        g = qsvg_hex2int(name + 3, &ok);
        b = qsvg_hex2int(name + 6, &ok);
    } else if (len == 6) {
        r = qsvg_hex2int(name, &ok);
        g = qsvg_hex2int(name + 2, &ok);
        b = qsvg_hex2int(name + 4, &ok);
    } else if (len == 3) {
        r = qsvg_hex2int(name[0], &ok);
        g = qsvg_hex2int(name[1], &ok);
        b = qsvg_hex2int(name[2], &ok);
    } else {
        r = g = b = -1;
    }
    if ((uint)r > 255 || (uint)g > 255 || (uint)b > 255 || !ok) {
        *rgb = 0;
        return false;
    }
    *rgb = qRgb(r, g ,b);
    return true;
}

bool qsvg_get_hex_rgb(const QChar *str, int len, QRgb *rgb)
{
    if (len > 13)
        return false;
    char tmp[16];
    for(int i = 0; i < len; ++i)
        tmp[i] = str[i].toLatin1();
    tmp[len] = 0;
    return qsvg_get_hex_rgb(tmp, rgb);
}

// ======== end of qcolor_p duplicate

static inline QString someId(const QXmlStreamAttributes &attributes)
{
    QStringView id = attributes.value(QLatin1String("id"));
    if (id.isEmpty())
        id = attributes.value(QLatin1String("xml:id"));
    return id.toString();
}

struct QSvgAttributes
{
    QSvgAttributes(const QXmlStreamAttributes &xmlAttributes, QSvgHandler *handler);
    void setAttributes(const QXmlStreamAttributes &attributes, QSvgHandler *handler);

    QString id;

    QStringView color;
    QStringView colorOpacity;
    QStringView fill;
    QStringView fillRule;
    QStringView fillOpacity;
    QStringView stroke;
    QStringView strokeDashArray;
    QStringView strokeDashOffset;
    QStringView strokeLineCap;
    QStringView strokeLineJoin;
    QStringView strokeMiterLimit;
    QStringView strokeOpacity;
    QStringView strokeWidth;
    QStringView vectorEffect;
    QStringView fontFamily;
    QStringView fontSize;
    QStringView fontStyle;
    QStringView fontWeight;
    QStringView fontVariant;
    QStringView textAnchor;
    QStringView transform;
    QStringView visibility;
    QStringView opacity;
    QStringView compOp;
    QStringView display;
    QStringView offset;
    QStringView stopColor;
    QStringView stopOpacity;
    QStringView imageRendering;
    QStringView mask;
    QStringView markerStart;
    QStringView markerMid;
    QStringView markerEnd;
    QStringView filter;
};

QSvgAttributes::QSvgAttributes(const QXmlStreamAttributes &xmlAttributes, QSvgHandler *handler)
{
    setAttributes(xmlAttributes, handler);
}

void QSvgAttributes::setAttributes(const QXmlStreamAttributes &attributes, QSvgHandler *handler)
{
    for (const QXmlStreamAttribute &attribute : attributes) {
        QStringView name = attribute.qualifiedName();
        if (name.isEmpty())
            continue;
        QStringView value = attribute.value();

        switch (name.at(0).unicode()) {

        case 'c':
            if (name == QLatin1String("color"))
                color = value;
            else if (name == QLatin1String("color-opacity"))
                colorOpacity = value;
            else if (name == QLatin1String("comp-op"))
                compOp = value;
            break;

        case 'd':
            if (name == QLatin1String("display"))
                display = value;
            break;

        case 'f':
            if (name == QLatin1String("fill"))
                fill = value;
            else if (name == QLatin1String("fill-rule"))
                fillRule = value;
            else if (name == QLatin1String("fill-opacity"))
                fillOpacity = value;
            else if (name == QLatin1String("font-family"))
                fontFamily = value;
            else if (name == QLatin1String("font-size"))
                fontSize = value;
            else if (name == QLatin1String("font-style"))
                fontStyle = value;
            else if (name == QLatin1String("font-weight"))
                fontWeight = value;
            else if (name == QLatin1String("font-variant"))
                fontVariant = value;
            else if (name == QLatin1String("filter") &&
                     !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
                filter = value;
            break;

        case 'i':
            if (name == QLatin1String("id"))
                id = value.toString();
            else if (name == QLatin1String("image-rendering"))
                imageRendering = value;
            break;

        case 'm':
            if (name == QLatin1String("mask") &&
                !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
                mask = value;
            if (name == QLatin1String("marker-start") &&
                !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
                markerStart = value;
            if (name == QLatin1String("marker-mid") &&
                !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
                markerMid = value;
            if (name == QLatin1String("marker-end") &&
                !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
                markerEnd = value;
            break;

        case 'o':
            if (name == QLatin1String("opacity"))
                opacity = value;
            if (name == QLatin1String("offset"))
                offset = value;
            break;

        case 's':
            if (name.size() > 5 && name.mid(1, 5) == QLatin1String("troke")) {
                QStringView strokeRef = name.mid(6, name.size() - 6);
                if (strokeRef.isEmpty())
                    stroke = value;
                else if (strokeRef == QLatin1String("-dasharray"))
                    strokeDashArray = value;
                else if (strokeRef == QLatin1String("-dashoffset"))
                    strokeDashOffset = value;
                else if (strokeRef == QLatin1String("-linecap"))
                    strokeLineCap = value;
                else if (strokeRef == QLatin1String("-linejoin"))
                    strokeLineJoin = value;
                else if (strokeRef == QLatin1String("-miterlimit"))
                    strokeMiterLimit = value;
                else if (strokeRef == QLatin1String("-opacity"))
                    strokeOpacity = value;
                else if (strokeRef == QLatin1String("-width"))
                    strokeWidth = value;
            } else if (name == QLatin1String("stop-color"))
                stopColor = value;
            else if (name == QLatin1String("stop-opacity"))
                stopOpacity = value;
            break;

        case 't':
            if (name == QLatin1String("text-anchor"))
                textAnchor = value;
            else if (name == QLatin1String("transform"))
                transform = value;
            break;

        case 'v':
            if (name == QLatin1String("vector-effect"))
                vectorEffect = value;
            else if (name == QLatin1String("visibility"))
                visibility = value;
            break;

        case 'x':
            if (name == QLatin1String("xml:id") && id.isEmpty())
                id = value.toString();
            break;

        default:
            break;
        }
    }
}

QList<qreal> parseNumbersList(QStringView *str)
{
    QList<qreal> points;
    if (!str)
        return points;
    points.reserve(32);

    while (!str->isEmpty() && str->first().isSpace())
        str->slice(1);
    while (!str->isEmpty()
           && (QGuiSvg::isDigit(str->first().unicode()) || str->startsWith(QLatin1Char('-'))
               || str->startsWith(QLatin1Char('+')) || str->startsWith(QLatin1Char('.')))) {

        points.append(QGuiSvg::toDouble(str));

        while (!str->isEmpty() && str->first().isSpace())
            str->slice(1);
        if (str->startsWith(QLatin1Char(',')))
            str->slice(1);

        //eat the rest of space
        while (!str->isEmpty() && str->first().isSpace())
            str->slice(1);
    }

    return points;
}

static QList<qreal> parsePercentageList(QStringView str)
{
    QList<qreal> points;

    while (!str.isEmpty() && str.first().isSpace())
        str.slice(1);
    while ((!str.isEmpty() && str.first() >= QLatin1Char('0') && str.first() <= QLatin1Char('9'))
           || str.startsWith(QLatin1Char('-')) || str.startsWith(QLatin1Char('+'))
           || str.startsWith(QLatin1Char('.'))) {

        points.append(QGuiSvg::toDouble(&str));

        while (!str.isEmpty() && str.first().isSpace())
            str.slice(1);
        if (str.startsWith(QLatin1Char('%')))
            str.slice(1);
        while (!str.isEmpty() && str.first().isSpace())
            str.slice(1);
        if (str.startsWith(QLatin1Char(',')))
            str.slice(1);

        //eat the rest of space
        while (!str.isEmpty() && str.first().isSpace())
            str.slice(1);
    }

    return points;
}

/**
 *  The form is <IRI>. This function parses local
 *  IRI references, i.e, resources referenced within
 *  the current document. e.g, href = "#id"
*/
static QStringView idFromIRI(QStringView iri)
{
    iri = iri.trimmed();

    if (!iri.startsWith(QLatin1Char('#')))
        return QStringView();

    return iri.sliced(1);
}

/**
 *  The form is <FuncIRI>, where FuncIRI takes
 *  the form of url(<IRI>). This syntax is used
 *  in properties that accept both strings and
 *  IRIs, eliminating any ambiguity. e.g, fill = "url(#id)"
*/
static QStringView idFromFuncIRI(QStringView iri)
{
    iri = iri.trimmed();

    if (!iri.startsWith(QLatin1StringView("url(")))
        return QStringView();

    iri.slice(4);

    const qsizetype closingBracePos = iri.indexOf(QLatin1Char(')'));
    if (closingBracePos == -1)
        return QStringView();

    iri = iri.first(closingBracePos);
    return idFromIRI(iri);
}

/**
 * returns true when successfully set the color. false signifies
 * that the color should be inherited
 */
bool resolveColor(QStringView colorStr, QColor &color, QSvgHandler *handler)
{
    QStringView colorStrTr = colorStr.trimmed();
    if (colorStrTr.isEmpty())
        return false;

    switch(colorStrTr.at(0).unicode()) {

        case '#':
            {
                // #rrggbb is very very common, so let's tackle it here
                // rather than falling back to QColor
                QRgb rgb;
                bool ok = qsvg_get_hex_rgb(colorStrTr.constData(), colorStrTr.size(), &rgb);
                if (ok)
                    color.setRgb(rgb);
                return ok;
            }
            break;

        case 'r':
            {
                // starts with "rgb(", ends with ")" and consists of at least 7 characters "rgb(,,)"
                if (colorStrTr.size() >= 7 && colorStrTr.at(colorStrTr.size() - 1) == QLatin1Char(')')
                    && colorStrTr.mid(0, 4) == QLatin1String("rgb(")) {
                    QStringView sv{ colorStrTr.sliced(4) };
                    QList<qreal> compo = parseNumbersList(&sv);
                    //1 means that it failed after reaching non-parsable
                    //character which is going to be "%"
                    if (compo.size() == 1) {
                        compo = parsePercentageList(colorStrTr.sliced(4));
                        for (int i = 0; i < compo.size(); ++i)
                            compo[i] *= (qreal)2.55;
                    }

                    if (compo.size() == 3) {
                        color = QColor(int(compo[0]),
                                       int(compo[1]),
                                       int(compo[2]));
                        return true;
                    }
                    return false;
                }
            }
            break;

        case 'c':
            if (colorStrTr == QLatin1String("currentColor")) {
                color = handler->currentColor();
                return true;
            }
            break;
        case 'i':
            if (colorStrTr == tokens::inherit)
                return false;
            break;
        default:
            break;
    }

    color = QColor::fromString(colorStrTr);
    return color.isValid();
}

void setAlpha(QStringView opacity, QColor *color)
{
    bool ok = true;
    qreal op = qBound(qreal(0.0), QGuiSvg::toDouble(opacity, &ok), qreal(1.0));
    if (!ok)
        op = 1.0;
    color->setAlphaF(op);
}

static bool constructColor(QStringView colorStr, QStringView opacity,
                           QColor &color, QSvgHandler *handler)
{
    if (!resolveColor(colorStr, color, handler))
        return false;
    if (!opacity.isEmpty())
        setAlpha(opacity, &color);
    return true;
}

static inline qreal convertToNumber(QStringView str, bool *ok = NULL)
{
    QGuiSvg::LengthType type;
    qreal num = QGuiSvg::parseLength(str.toString(), &type, ok);
    if (type == QGuiSvg::LengthType::LT_PERCENT) {
        num = num/100.0;
    }
    return num;
}

static bool createSvgGlyph(QSvgFont *font, const QXmlStreamAttributes &attributes,
                           bool isMissingGlyph)
{
    QStringView uncStr = attributes.value(QLatin1String("unicode"));
    QStringView havStr = attributes.value(QLatin1String("horiz-adv-x"));
    QStringView pathStr = attributes.value(QLatin1String("d"));

    qreal havx = (havStr.isEmpty()) ? -1 : QGuiSvg::toDouble(havStr);
    QPainterPath path = QGuiSvg::parsePath(pathStr).value_or(QPainterPath());

    path.setFillRule(Qt::WindingFill);

    if (isMissingGlyph) {
        if (!uncStr.isEmpty())
            qWarning("Ignoring missing-glyph's 'unicode' attribute");
        return font->addMissingGlyph(path, havx);
    }

    if (uncStr.isEmpty()) {
        qWarning("glyph does not define a non-empty 'unicode' attribute and will be ignored");
        return false;
    }
    font->addGlyph(uncStr.toString(), path, havx);
    return true;
}

static void parseColor(QSvgNode *,
                       const QSvgAttributes &attributes,
                       QSvgHandler *handler)
{
    QColor color;
    if (constructColor(attributes.color, attributes.colorOpacity, color, handler)) {
        handler->popColor();
        handler->pushColor(color);
    }
}

static QSvgPaintServerSharedPtr paintServerFromUrl(QSvgDocument *doc, QStringView url)
{
    QStringView id = idFromFuncIRI(url);
    return doc ? doc->paintServer(id) : nullptr;
}

static void parseBrush(QSvgNode *node,
                       const QSvgAttributes &attributes,
                       QSvgHandler *handler)
{
    if (!attributes.fill.isEmpty() || !attributes.fillRule.isEmpty() || !attributes.fillOpacity.isEmpty()) {
        QSvgFillStylePtr prop = std::make_unique<QSvgFillStyle>();

        //fill-rule attribute handling
        if (!attributes.fillRule.isEmpty() && attributes.fillRule != tokens::inherit) {
            if (attributes.fillRule == QLatin1String("evenodd"))
                prop->setFillRule(Qt::OddEvenFill);
            else if (attributes.fillRule == QLatin1String("nonzero"))
                prop->setFillRule(Qt::WindingFill);
        }

        //fill-opacity attribute handling
        if (!attributes.fillOpacity.isEmpty() && attributes.fillOpacity != tokens::inherit) {
            prop->setFillOpacity(qMin(qreal(1.0), qMax(qreal(0.0), QGuiSvg::toDouble(attributes.fillOpacity))));
        }

        //fill attribute handling
        if (!attributes.fill.isEmpty() && attributes.fill != tokens::inherit) {
            if (attributes.fill.startsWith(QLatin1String("url"))) {
                QStringView value = attributes.fill;
                QSvgPaintServerSharedPtr paintServer = paintServerFromUrl(handler->document(), value);
                if (paintServer) {
                    prop->setPaintServer(std::move(paintServer));
                } else {
                    QString id = idFromFuncIRI(value).toString();
                    prop->setPaintStyleId(id);
                    handler->pushUnresolvedStyle(prop.get());
                }
            } else if (attributes.fill != QLatin1String("none")) {
                QColor color;
                if (resolveColor(attributes.fill, color, handler))
                    prop->setBrush(QBrush(color));
            } else {
                prop->setBrush(QBrush(Qt::NoBrush));
            }
        }
        node->appendStyleProperty(std::move(prop));
    }
}



static QTransform parseTransformationMatrix(QStringView value)
{
    if (value.isEmpty())
        return QTransform();

    QTransform matrix;

    while (!value.isEmpty()) {
        if (value.first().isSpace() || value.startsWith(QLatin1Char(','))) {
            value.slice(1);
            continue;
        }
        enum State {
            Matrix,
            Translate,
            Rotate,
            Scale,
            SkewX,
            SkewY
        };
        State state = Matrix;
        if (value.startsWith(QLatin1Char('m'))) { //matrix
            const char *ident = "atrix";
            for (int i = 0; i < 5; ++i)
                if (!value.slice(1).startsWith(QLatin1Char(ident[i])))
                    goto error;
            value.slice(1);
            state = Matrix;
        } else if (value.startsWith(QLatin1Char('t'))) { //translate
            const char *ident = "ranslate";
            for (int i = 0; i < 8; ++i)
                if (!value.slice(1).startsWith(QLatin1Char(ident[i])))
                    goto error;
            value.slice(1);
            state = Translate;
        } else if (value.startsWith(QLatin1Char('r'))) { //rotate
            const char *ident = "otate";
            for (int i = 0; i < 5; ++i)
                if (!value.slice(1).startsWith(QLatin1Char(ident[i])))
                    goto error;
            value.slice(1);
            state = Rotate;
        } else if (value.startsWith(QLatin1Char('s'))) { //scale, skewX, skewY
            value.slice(1);
            if (value.startsWith(QLatin1Char('c'))) {
                const char *ident = "ale";
                for (int i = 0; i < 3; ++i)
                    if (!value.slice(1).startsWith(QLatin1Char(ident[i])))
                        goto error;
                value.slice(1);
                state = Scale;
            } else if (value.startsWith(QLatin1Char('k'))) {
                if (!value.slice(1).startsWith(QLatin1Char('e')))
                    goto error;
                if (!value.slice(1).startsWith(QLatin1Char('w')))
                    goto error;
                value.slice(1);
                if (value.startsWith(QLatin1Char('X')))
                    state = SkewX;
                else if (value.startsWith(QLatin1Char('Y')))
                    state = SkewY;
                else
                    goto error;
                value.slice(1);
            } else {
                goto error;
            }
        } else {
            goto error;
        }

        while (!value.isEmpty() && value.first().isSpace())
            value.slice(1);
        if (!value.startsWith(QLatin1Char('(')))
            goto error;
        value.slice(1);
        QVarLengthArray<qreal, 8> points;
        QGuiSvg::parseNumbersArray(&value, points);
        if (!value.startsWith(QLatin1Char(')')))
            goto error;
        value.slice(1);

        if(state == Matrix) {
            if(points.size() != 6)
                goto error;
            matrix = QTransform(points[0], points[1],
                                points[2], points[3],
                                points[4], points[5]) * matrix;
        } else if (state == Translate) {
            if (points.size() == 1)
                matrix.translate(points[0], 0);
            else if (points.size() == 2)
                matrix.translate(points[0], points[1]);
            else
                goto error;
        } else if (state == Rotate) {
            if(points.size() == 1) {
                matrix.rotate(points[0]);
            } else if (points.size() == 3) {
                matrix.translate(points[1], points[2]);
                matrix.rotate(points[0]);
                matrix.translate(-points[1], -points[2]);
            } else {
                goto error;
            }
        } else if (state == Scale) {
            if (points.size() < 1 || points.size() > 2)
                goto error;
            qreal sx = points[0];
            qreal sy = sx;
            if(points.size() == 2)
                sy = points[1];
            matrix.scale(sx, sy);
        } else if (state == SkewX) {
            if (points.size() != 1)
                goto error;
            matrix.shear(qTan(qDegreesToRadians(points[0])), 0);
        } else if (state == SkewY) {
            if (points.size() != 1)
                goto error;
            matrix.shear(0, qTan(qDegreesToRadians(points[0])));
        }
    }
  error:
    return matrix;
}

static void parsePen(QSvgNode *node,
                     const QSvgAttributes &attributes,
                     QSvgHandler *handler)
{
    if (!attributes.stroke.isEmpty() || !attributes.strokeDashArray.isEmpty() || !attributes.strokeDashOffset.isEmpty() || !attributes.strokeLineCap.isEmpty()
        || !attributes.strokeLineJoin.isEmpty() || !attributes.strokeMiterLimit.isEmpty() || !attributes.strokeOpacity.isEmpty() || !attributes.strokeWidth.isEmpty()
        || !attributes.vectorEffect.isEmpty()) {

        QSvgStrokeStylePtr prop = std::make_unique<QSvgStrokeStyle>();

        //stroke attribute handling
        if (!attributes.stroke.isEmpty() && attributes.stroke != tokens::inherit) {
            if (attributes.stroke.startsWith(QLatin1String("url"))) {
                QStringView value = attributes.stroke;
                QSvgPaintServerSharedPtr paintServer = paintServerFromUrl(handler->document(), value);
                if (paintServer) {
                    prop->setPaintServer(std::move(paintServer));
                } else {
                    QString id = idFromFuncIRI(value).toString();
                    prop->setPaintStyleId(id);
                    handler->pushUnresolvedStyle(prop.get());
                }
            } else if (attributes.stroke != QLatin1String("none")) {
                QColor color;
                if (resolveColor(attributes.stroke, color, handler))
                    prop->setStroke(QBrush(color));
            } else {
                prop->setStroke(QBrush(Qt::NoBrush));
            }
        }

        //stroke-width handling
        if (!attributes.strokeWidth.isEmpty() && attributes.strokeWidth != tokens::inherit) {
            QGuiSvg::LengthType lt;
            prop->setWidth(QGuiSvg::parseLength(attributes.strokeWidth, &lt));
        }

        //stroke-dasharray
        if (!attributes.strokeDashArray.isEmpty() && attributes.strokeDashArray != tokens::inherit) {
            if (attributes.strokeDashArray == QLatin1String("none")) {
                prop->setDashArrayNone();
            } else {
                QStringView dashArray = attributes.strokeDashArray;
                QList<qreal> dashes = parseNumbersList(&dashArray);
                const bool allZeroes = std::all_of(dashes.cbegin(), dashes.cend(),
                                                   [](qreal i) { return qFuzzyIsNull(i); });
                const bool hasNegative = !allZeroes && std::any_of(dashes.cbegin(), dashes.cend(),
                                                                   [](qreal i) { return i < 0.; });

                if (hasNegative)
                    qCWarning(lcSvgHandler) << "QSvgHandler: Stroke dash array "
                                               "with a negative value is invalid";
                // if the stroke dash array contains only zeros or a negative value,
                // force drawing of solid line.
                if (allZeroes || hasNegative) {
                    prop->setDashArrayNone();
                } else {
                    // if the dash count is odd the dashes should be duplicated
                    if ((dashes.size() & 1) != 0)
                        dashes << QList<qreal>(dashes);
                    prop->setDashArray(dashes);
                }
            }
        }

        //stroke-linejoin attribute handling
        if (!attributes.strokeLineJoin.isEmpty()) {
            if (attributes.strokeLineJoin == QLatin1String("miter"))
                prop->setLineJoin(Qt::SvgMiterJoin);
            else if (attributes.strokeLineJoin == QLatin1String("round"))
                prop->setLineJoin(Qt::RoundJoin);
            else if (attributes.strokeLineJoin == QLatin1String("bevel"))
                prop->setLineJoin(Qt::BevelJoin);
        }

        //stroke-linecap attribute handling
        if (!attributes.strokeLineCap.isEmpty()) {
            if (attributes.strokeLineCap == QLatin1String("butt"))
                prop->setLineCap(Qt::FlatCap);
            else if (attributes.strokeLineCap == QLatin1String("round"))
                prop->setLineCap(Qt::RoundCap);
            else if (attributes.strokeLineCap == QLatin1String("square"))
                prop->setLineCap(Qt::SquareCap);
        }

        //stroke-dashoffset attribute handling
        if (!attributes.strokeDashOffset.isEmpty() && attributes.strokeDashOffset != tokens::inherit)
            prop->setDashOffset(QGuiSvg::toDouble(attributes.strokeDashOffset));

        //vector-effect attribute handling
        if (!attributes.vectorEffect.isEmpty()) {
            if (attributes.vectorEffect == QLatin1String("non-scaling-stroke"))
                prop->setVectorEffect(true);
            else if (attributes.vectorEffect == QLatin1String("none"))
                prop->setVectorEffect(false);
        }

        //stroke-miterlimit
        if (!attributes.strokeMiterLimit.isEmpty() && attributes.strokeMiterLimit != tokens::inherit)
            prop->setMiterLimit(QGuiSvg::toDouble(attributes.strokeMiterLimit));

        //stroke-opacity atttribute handling
        if (!attributes.strokeOpacity.isEmpty() && attributes.strokeOpacity != tokens::inherit)
            prop->setOpacity(qMin(qreal(1.0), qMax(qreal(0.0), QGuiSvg::toDouble(attributes.strokeOpacity))));

        node->appendStyleProperty(std::move(prop));
    }
}

enum FontSizeSpec { XXSmall, XSmall, Small, Medium, Large, XLarge, XXLarge,
                   FontSizeNone, FontSizeValue };

static const qreal sizeTable[] =
{ qreal(6.9), qreal(8.3), qreal(10.0), qreal(12.0), qreal(14.4), qreal(17.3), qreal(20.7) };

Q_STATIC_ASSERT(sizeof(sizeTable)/sizeof(sizeTable[0]) == FontSizeNone);

static FontSizeSpec fontSizeSpec(QStringView spec)
{
    switch (spec.at(0).unicode()) {
    case 'x':
        if (spec == QLatin1String("xx-small"))
            return XXSmall;
        if (spec == QLatin1String("x-small"))
            return XSmall;
        if (spec == QLatin1String("x-large"))
            return XLarge;
        if (spec == QLatin1String("xx-large"))
            return XXLarge;
        break;
    case 's':
        if (spec == QLatin1String("small"))
            return Small;
        break;
    case 'm':
        if (spec == QLatin1String("medium"))
            return Medium;
        break;
    case 'l':
        if (spec == QLatin1String("large"))
            return Large;
        break;
    case 'n':
        if (spec == QLatin1String("none"))
            return FontSizeNone;
        break;
    default:
        break;
    }
    return FontSizeValue;
}

static std::optional<QFont::Style> parseFontStyle(QStringView s)
{
    // https://www.w3.org/TR/2018/REC-css-fonts-3-20180920/#font-style-prop
    //   Value: normal | italic | oblique

    if (s == tokens::normal)
        return QFont::StyleNormal;
    if (s == tokens::italic)
        return QFont::StyleItalic;
    if (s == tokens::oblique)
        return QFont::StyleOblique;

    return std::nullopt; // incl. empty and tokens::inherit
}

static std::optional<qreal> parseFontSize(QStringView s)
{
    // https://www.w3.org/TR/2018/REC-css-fonts-3-20180920/#font-size-prop
    //   Value:           <absolute-size> | <relative-size> | <length-percentage>
    //   <absolute-size>: [ xx-small | x-small | small | medium | large | x-large | xx-large ]
    //   <relative-size>: [ larger | smaller ]

    // TODO: Support <relative-size>s

    if (s.isEmpty() || s == tokens::inherit)
        return std::nullopt;

    const FontSizeSpec spec = fontSizeSpec(s);
    switch (spec) {
    case FontSizeNone:
        return std::nullopt;
    case FontSizeValue: {
        QGuiSvg::LengthType type;
        bool ok = false;
        qreal fs = QGuiSvg::parseLength(s, &type, &ok);
        if (!ok)
            return std::nullopt;
        fs = QGuiSvg::convertToPixels(fs, true, type);
        return (std::min)(fs, qreal(0xffff));
    }
    case XXSmall:
    case XSmall:
    case Small:
    case Medium:
    case Large:
    case XLarge:
    case XXLarge:
        return sizeTable[spec];
    }

    Q_UNREACHABLE_RETURN(std::nullopt);
}

static std::optional<int> parseFontWeight(QStringView s)
{
    // https://www.w3.org/TR/2018/REC-css-fonts-3-20180920/#font-weight-prop
    //   Value: normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900

    if (s.isEmpty() || s == tokens::inherit)
        return std::nullopt;

    if (s == tokens::normal)
        return QFont::Normal;
    if (s == tokens::bold)
        return QFont::Bold;
    if (s == tokens::bolder)
        return QSvgFontStyle::BOLDER;
    if (s == tokens::lighter)
        return QSvgFontStyle::LIGHTER;

    bool ok = false;
    const int num = s.toInt(&ok);
    if (ok)
        return num;

    return std::nullopt;
}

static std::optional<QFont::Capitalization> parseFontVariant(const QSvgAttributes &attributes)
{
    // https://www.w3.org/TR/2018/REC-css-fonts-3-20180920/#font-variant-prop
    //   Value: normal |
    //          none |
    //          [
    //              <common-lig-values> ||
    //              <discretionary-lig-values> ||
    //              <historical-lig-values> ||
    //              <contextual-alt-values> ||
    //              [ small-caps | all-small-caps | petite-caps | all-petite-caps | unicase | titling-caps ] ||
    //              <numeric-figure-values> ||
    //              <numeric-spacing-values> ||
    //              <numeric-fraction-values> ||
    //              ordinal ||
    //              slashed-zero ||
    //              <east-asian-variant-values> ||
    //              <east-asian-width-values> ||
    //              ruby ||
    //              [ sub | super ]
    //          ]

    // TODO: implement parsing of sub-properties, and values other than normal and small-caps

    auto s = attributes.fontVariant;

    if (s == tokens::normal)
        return QFont::MixedCase;
    if (s == tokens::small_caps)
        return QFont::SmallCaps;

    return std::nullopt; // incl. empty and tokens::inherit
}

static std::optional<Qt::Alignment> parseTextAnchor(QStringView s)
{
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/Properties/text-anchor#formal_syntax
    //   text-anchor =
    //      start   |
    //      middle  |
    //      end

    if (s == tokens::start)
        return Qt::AlignLeft;
    if (s == tokens::middle)
        return Qt::AlignHCenter;
    if (s == tokens::end)
        return Qt::AlignRight;

    return std::nullopt; // incl. empty and tokens::inherit
}

static void parseFont(QSvgNode *node,
                      const QSvgAttributes &attributes,
                      QSvgHandler *)
{
    auto parsedFontSize = parseFontSize(attributes.fontSize);
    auto parsedFontStyle = parseFontStyle(attributes.fontStyle);
    auto parsedFontWeight = parseFontWeight(attributes.fontWeight);
    auto parsedFontVariant = parseFontVariant(attributes);
    auto parsedTextAnchor = parseTextAnchor(attributes.textAnchor);

    if (attributes.fontFamily.isEmpty() && !parsedFontSize && !parsedFontStyle &&
        !parsedFontWeight && !parsedFontVariant && !parsedTextAnchor)
        return;

    QSvgFontStylePtr fontStyle = std::make_unique<QSvgFontStyle>();

    if (!attributes.fontFamily.isEmpty() && attributes.fontFamily != tokens::inherit) {
        QStringView family = attributes.fontFamily.trimmed();
        if (!family.isEmpty() && (family.at(0) == QLatin1Char('\'') || family.at(0) == QLatin1Char('\"')))
            family = family.mid(1, family.size() - 2);
        fontStyle->setFamily(family.toString());
    }

    if (parsedFontSize)
        fontStyle->setSize(*parsedFontSize);

    if (parsedFontStyle)
        fontStyle->setStyle(*parsedFontStyle);

    if (parsedFontWeight)
        fontStyle->setWeight(*parsedFontWeight);

    if (parsedFontVariant)
        fontStyle->setVariant(*parsedFontVariant);

    if (parsedTextAnchor)
        fontStyle->setTextAnchor(*parsedTextAnchor);

    node->appendStyleProperty(std::move(fontStyle));
}

static void parseTransform(QSvgNode *node,
                           const QSvgAttributes &attributes,
                           QSvgHandler *)
{
    if (attributes.transform.isEmpty())
        return;
    QTransform matrix = parseTransformationMatrix(attributes.transform.trimmed());

    if (!matrix.isIdentity()) {
        node->appendStyleProperty(std::make_unique<QSvgTransformStyle>(QTransform(matrix)));
    }

}

static void parseVisibility(QSvgNode *node,
                            const QSvgAttributes &attributes,
                            QSvgHandler *)
{
    QSvgNode *parent = node->parent();

    if (parent && (attributes.visibility.isEmpty() || attributes.visibility == tokens::inherit))
        node->setVisible(parent->isVisible());
    else if (attributes.visibility == QLatin1String("hidden") || attributes.visibility == QLatin1String("collapse")) {
        node->setVisible(false);
    } else
        node->setVisible(true);
}

static bool parseStyle(QSvgNode *node,
                       const QXmlStreamAttributes &attributes,
                       QSvgHandler *handler);

static int parseClockValue(QStringView str, bool *ok)
{
    int res = 0;
    int ms = 1000;
    str = str.trimmed();
    if (str.endsWith(QLatin1String("ms"))) {
        str.chop(2);
        ms = 1;
    } else if (str.endsWith(QLatin1String("s"))) {
        str.chop(1);
    }
    double val = ms * QGuiSvg::toDouble(str, ok);
    if (ok) {
        if (val > std::numeric_limits<int>::min() && val < std::numeric_limits<int>::max())
            res = static_cast<int>(val);
        else
            *ok = false;
    }
    return res;
}

#ifndef QT_NO_CSSPARSER

static void parseCssAnimations(QSvgNode *node,
                               const QXmlStreamAttributes &attributes,
                               QSvgHandler *handler)
{
    QSvgCssProperties cssAnimProps(attributes);
    QList<QSvgAnimationProperty> parsedProperties = cssAnimProps.animations();

    for (auto &property : parsedProperties) {
        QSvgCssAnimation *anim = handler->cssHandler().createAnimation(property.name);
        if (!anim)
            continue;

        anim->setRunningTime(property.delay, property.duration);
        anim->setIterationCount(property.iteration);
        QSvgCssEasingPtr easing = handler->cssHandler().createEasing(property.easingFunction, property.easingValues);
        anim->setEasing(std::move(easing));

        handler->setAnimPeriod(property.delay, property.delay + property.duration);
        handler->document()->animator()->appendAnimation(node, anim);
        handler->document()->setAnimated(true);
    }
}

static void parseOffsetPath(QSvgNode *node,
                            const QXmlStreamAttributes &attributes)
{
    QSvgCssProperties cssProperties(attributes);
    QSvgOffsetProperty offset = cssProperties.offset();

    if (!offset.path)
        return;

    QSvgOffsetStylePtr offsetStyle = std::make_unique<QSvgOffsetStyle>();
    offsetStyle->setPath(offset.path.value());
    offsetStyle->setRotateAngle(offset.angle);
    offsetStyle->setRotateType(offset.rotateType);
    offsetStyle->setDistance(offset.distance);
    node->appendStyleProperty(std::move(offsetStyle));
}

#endif // QT_NO_CSSPARSER

QtSvg::Options QSvgHandler::options() const
{
    return m_options;
}

QtSvg::AnimatorType QSvgHandler::animatorType() const
{
    return m_animatorType;
}

bool QSvgHandler::trustedSourceMode() const
{
    return m_options.testFlag(QtSvg::AssumeTrustedSource);
}

static inline QStringList stringToList(const QString &str)
{
    QStringList lst = str.split(QLatin1Char(','), Qt::SkipEmptyParts);
    return lst;
}

static bool parseCoreNode(QSvgNode *node,
                          const QXmlStreamAttributes &attributes)
{
    QStringList features;
    QStringList extensions;
    QStringList languages;
    QStringList formats;
    QStringList fonts;
    QStringView xmlClassStr;

    for (const QXmlStreamAttribute &attribute : attributes) {
        QStringView name = attribute.qualifiedName();
        if (name.isEmpty())
            continue;
        QStringView value = attribute.value();
        switch (name.at(0).unicode()) {
        case 'c':
            if (name == QLatin1String("class"))
                xmlClassStr = value;
            break;
        case 'r':
            if (name == QLatin1String("requiredFeatures"))
                features = stringToList(value.toString());
            else if (name == QLatin1String("requiredExtensions"))
                extensions = stringToList(value.toString());
            else if (name == QLatin1String("requiredFormats"))
                formats = stringToList(value.toString());
            else if (name == QLatin1String("requiredFonts"))
                fonts = stringToList(value.toString());
            break;
        case 's':
            if (name == QLatin1String("systemLanguage"))
                languages = stringToList(value.toString());
            break;
        default:
            break;
        }
    }

    node->setRequiredFeatures(features);
    node->setRequiredExtensions(extensions);
    node->setRequiredLanguages(languages);
    node->setRequiredFormats(formats);
    node->setRequiredFonts(fonts);
    node->setNodeId(someId(attributes));
    node->setXmlClass(xmlClassStr.toString());

    return true;
}

static void parseOpacity(QSvgNode *node,
                         const QSvgAttributes &attributes,
                         QSvgHandler *)
{
    if (attributes.opacity.isEmpty())
        return;

    const QStringView value = attributes.opacity.trimmed();

    bool ok = false;
    qreal op = value.toDouble(&ok);

    if (ok) {
        QSvgOpacityStylePtr opacity = std::make_unique<QSvgOpacityStyle>(qBound(qreal(0.0), op, qreal(1.0)));
        node->appendStyleProperty(std::move(opacity));
    }
}

static QPainter::CompositionMode svgToQtCompositionMode(const QStringView op)
{
    if (op == tokens::compOp::clear)
        return QPainter::CompositionMode_Clear;
    else if (op == tokens::compOp::src)
        return QPainter::CompositionMode_Source;
    else if (op == tokens::compOp::dst)
        return QPainter::CompositionMode_Destination;
    else if (op == tokens::compOp::srcOver)
        return QPainter::CompositionMode_SourceOver;
    else if (op == tokens::compOp::dstOver)
        return QPainter::CompositionMode_DestinationOver;
    else if (op == tokens::compOp::srcIn)
        return QPainter::CompositionMode_SourceIn;
    else if (op == tokens::compOp::dstIn)
        return QPainter::CompositionMode_DestinationIn;
    else if (op == tokens::compOp::srcOut)
        return QPainter::CompositionMode_SourceOut;
    else if (op == tokens::compOp::dstOut)
        return QPainter::CompositionMode_DestinationOut;
    else if (op == tokens::compOp::srcAtop)
        return QPainter::CompositionMode_SourceAtop;
    else if (op == tokens::compOp::dstAtop)
        return QPainter::CompositionMode_DestinationAtop;
    else if (op == tokens::compOp::xorOp)
        return QPainter::CompositionMode_Xor;
    else if (op == tokens::compOp::plus)
        return QPainter::CompositionMode_Plus;
    else if (op == tokens::compOp::multiply)
        return QPainter::CompositionMode_Multiply;
    else if (op == tokens::compOp::screen)
        return QPainter::CompositionMode_Screen;
    else if (op == tokens::compOp::overlay)
        return QPainter::CompositionMode_Overlay;
    else if (op == tokens::compOp::darken)
        return QPainter::CompositionMode_Darken;
    else if (op == tokens::compOp::lighten)
        return QPainter::CompositionMode_Lighten;
    else if (op == tokens::compOp::colorDodge)
        return QPainter::CompositionMode_ColorDodge;
    else if (op == tokens::compOp::colorBurn)
        return QPainter::CompositionMode_ColorBurn;
    else if (op == tokens::compOp::hardLight)
        return QPainter::CompositionMode_HardLight;
    else if (op == tokens::compOp::softLight)
        return QPainter::CompositionMode_SoftLight;
    else if (op == tokens::compOp::difference)
        return QPainter::CompositionMode_Difference;
    else if (op == tokens::compOp::exclusion)
        return QPainter::CompositionMode_Exclusion;

    qCWarning(lcSvgHandler) << "Composition mode not supported : "_L1 << op;
    return QPainter::CompositionMode_SourceOver;
}

static void parseCompOp(QSvgNode *node,
                        const QSvgAttributes &attributes,
                        QSvgHandler *)
{
    if (attributes.compOp.isEmpty())
        return;
    QStringView value = attributes.compOp.trimmed();

    if (!value.isEmpty()) {
        QSvgCompOpStylePtr compop = std::make_unique<QSvgCompOpStyle>(svgToQtCompositionMode(value));
        node->appendStyleProperty(std::move(compop));
    }
}

static QSvgNode::DisplayMode displayStringToEnum(const QStringView str)
{
    if (str == QLatin1String("inline")) {
        return QSvgNode::InlineMode;
    } else if (str == QLatin1String("block")) {
        return QSvgNode::BlockMode;
    } else if (str == QLatin1String("list-item")) {
        return QSvgNode::ListItemMode;
    } else if (str == QLatin1String("run-in")) {
        return QSvgNode::RunInMode;
    } else if (str == QLatin1String("compact")) {
        return QSvgNode::CompactMode;
    } else if (str == QLatin1String("marker")) {
        return QSvgNode::MarkerMode;
    } else if (str == QLatin1String("table")) {
        return QSvgNode::TableMode;
    } else if (str == QLatin1String("inline-table")) {
        return QSvgNode::InlineTableMode;
    } else if (str == QLatin1String("table-row-group")) {
        return QSvgNode::TableRowGroupMode;
    } else if (str == QLatin1String("table-header-group")) {
        return QSvgNode::TableHeaderGroupMode;
    } else if (str == QLatin1String("table-footer-group")) {
        return QSvgNode::TableFooterGroupMode;
    } else if (str == QLatin1String("table-row")) {
        return QSvgNode::TableRowMode;
    } else if (str == QLatin1String("table-column-group")) {
        return QSvgNode::TableColumnGroupMode;
    } else if (str == QLatin1String("table-column")) {
        return QSvgNode::TableColumnMode;
    } else if (str == QLatin1String("table-cell")) {
        return QSvgNode::TableCellMode;
    } else if (str == QLatin1String("table-caption")) {
        return QSvgNode::TableCaptionMode;
    } else if (str == QLatin1String("none")) {
        return QSvgNode::NoneMode;
    } else if (str == tokens::inherit) {
        return QSvgNode::InheritMode;
    }
    return QSvgNode::BlockMode;
}

static void parseOthers(QSvgNode *node,
                        const QSvgAttributes &attributes,
                        QSvgHandler *)
{
    if (attributes.display.isEmpty())
        return;
    QStringView displayStr = attributes.display.trimmed();

    if (!displayStr.isEmpty()) {
        node->setDisplayMode(displayStringToEnum(displayStr));
    }
}

static std::optional<QStringView> getAttributeId(const QStringView &attribute)
{
    if (attribute.isEmpty())
        return std::nullopt;

    return idFromFuncIRI(attribute);
}

static void parseExtendedAttributes(QSvgNode *node,
                                    const QSvgAttributes &attributes,
                                    QSvgHandler *handler)
{
    if (handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
        return;

    if (auto id = getAttributeId(attributes.mask))
        node->setMaskId(id->toString());
    if (auto id = getAttributeId(attributes.markerStart))
        node->setMarkerStartId(id->toString());
    if (auto id = getAttributeId(attributes.markerMid))
        node->setMarkerMidId(id->toString());
    if (auto id = getAttributeId(attributes.markerEnd))
        node->setMarkerEndId(id->toString());
    if (auto id = getAttributeId(attributes.filter))
        node->setFilterId(id->toString());
}

static void parseRenderingHints(QSvgNode *node,
                                const QSvgAttributes &attributes,
                                QSvgHandler *)
{
    if (attributes.imageRendering.isEmpty())
        return;

    QStringView ir = attributes.imageRendering.trimmed();
    QSvgQualityStylePtr quality = std::make_unique<QSvgQualityStyle>(0);
    if (ir == QLatin1String("auto"))
        quality->setImageRendering(QSvgQualityStyle::ImageRenderingAuto);
    else if (ir == QLatin1String("optimizeSpeed"))
        quality->setImageRendering(QSvgQualityStyle::ImageRenderingOptimizeSpeed);
    else if (ir == QLatin1String("optimizeQuality"))
        quality->setImageRendering(QSvgQualityStyle::ImageRenderingOptimizeQuality);
    node->appendStyleProperty(std::move(quality));
}

static bool parseStyle(QSvgNode *node,
                       const QXmlStreamAttributes &attributes,
                       QSvgHandler *handler)
{
    // Get style in the following order :
    // 1) values from svg attributes
    // 2) CSS style
    // 3) values defined in the svg "style" property
    QSvgAttributes svgAttributes(attributes, handler);

#ifndef QT_NO_CSSPARSER
    QXmlStreamAttributes cssAttributes;
    handler->cssHandler().styleLookup(node, cssAttributes);

    QStringView style = attributes.value(QLatin1String("style"));
    if (!style.isEmpty())
        handler->cssHandler().parseCSStoXMLAttrs(style.toString(), cssAttributes);
    svgAttributes.setAttributes(cssAttributes, handler);

    parseOffsetPath(node, cssAttributes);
    if (!handler->options().testFlag(QtSvg::DisableCSSAnimations))
        parseCssAnimations(node, cssAttributes, handler);
#endif

    parseColor(node, svgAttributes, handler);
    parseBrush(node, svgAttributes, handler);
    parsePen(node, svgAttributes, handler);
    parseFont(node, svgAttributes, handler);
    parseTransform(node, svgAttributes, handler);
    parseVisibility(node, svgAttributes, handler);
    parseOpacity(node, svgAttributes, handler);
    parseCompOp(node, svgAttributes, handler);
    parseRenderingHints(node, svgAttributes, handler);
    parseOthers(node, svgAttributes, handler);
    parseExtendedAttributes(node, svgAttributes, handler);

    return true;
}

static bool parseAnchorNode(QSvgNode *parent,
                            const QXmlStreamAttributes &attributes,
                            QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseBaseAnimate(QSvgNode *,
                             const QXmlStreamAttributes &attributes,
                             QSvgAnimateNode *anim,
                             QSvgHandler *handler)
{
    const QStringView beginStr   = attributes.value(QLatin1String("begin"));
    const QStringView durStr     = attributes.value(QLatin1String("dur"));
    const QStringView endStr     = attributes.value(QLatin1String("end"));
    const QStringView repeatStr  = attributes.value(QLatin1String("repeatCount"));
    const QStringView fillStr    = attributes.value(QLatin1String("fill"));
    const QStringView addtv      = attributes.value(QLatin1String("additive"));
    QStringView linkId           = attributes.value(QLatin1String("xlink:href"));

    if (linkId.isEmpty())
        linkId = attributes.value(QLatin1String("href"));

    linkId = idFromIRI(linkId);

    bool ok = true;
    int begin = parseClockValue(beginStr, &ok);
    if (!ok)
        return false;
    int dur = parseClockValue(durStr, &ok);
    if (!ok)
        return false;
    int end = parseClockValue(endStr, &ok);
    if (!ok)
        return false;
    qreal repeatCount = (repeatStr == QLatin1String("indefinite")) ? -1 :
                            qMax(1.0, QGuiSvg::toDouble(repeatStr));

    QSvgAnimateNode::Fill fill = (fillStr == QLatin1String("freeze")) ? QSvgAnimateNode::Freeze :
                                     QSvgAnimateNode::Remove;

    QSvgAnimateNode::Additive additive = (addtv == QLatin1String("sum")) ? QSvgAnimateNode::Sum :
                                             QSvgAnimateNode::Replace;

    anim->setRunningTime(begin, dur, end, 0);
    anim->setRepeatCount(repeatCount);
    anim->setFill(fill);
    anim->setAdditiveType(additive);
    anim->setLinkId(linkId.toString());

    handler->document()->setAnimated(true);

    handler->setAnimPeriod(begin, begin + dur);
    return true;
}

static void generateKeyFrames(QList<qreal> &keyFrames, uint count)
{
    if (count < 2)
        return;

    qreal spacing = 1.0f / (count - 1);
    for (uint i = 0; i < count; i++) {
        keyFrames.append(i * spacing);
    }
}

static QSvgNode *createAnimateColorNode(QSvgNode *parent,
                                        const QXmlStreamAttributes &attributes,
                                        QSvgHandler *handler)
{
    const QStringView fromStr   = attributes.value(QLatin1String("from"));
    const QStringView toStr     = attributes.value(QLatin1String("to"));
    const QStringView valuesStr = attributes.value(QLatin1String("values"));
    const QString targetStr     = attributes.value(QLatin1String("attributeName")).toString();

    if (targetStr != QLatin1String("fill") && targetStr != QLatin1String("stroke"))
        return nullptr;

    QList<QColor> colors;
    if (valuesStr.isEmpty()) {
        QColor startColor, endColor;
        resolveColor(fromStr, startColor, handler);
        resolveColor(toStr, endColor, handler);
        colors.reserve(2);
        colors.append(startColor);
        colors.append(endColor);
    } else {
        for (auto part : qTokenize(valuesStr, u';')) {
            QColor color;
            resolveColor(part, color, handler);
            colors.append(color);
        }
    }

    QSvgAnimatedPropertyColor *prop = static_cast<QSvgAnimatedPropertyColor *>
                                            (QSvgAbstractAnimatedProperty::createAnimatedProperty(targetStr));
    if (!prop)
        return nullptr;

    prop->setColors(colors);

    QList<qreal> keyFrames;
    generateKeyFrames(keyFrames, colors.size());
    prop->setKeyFrames(keyFrames);

    QSvgAnimateColor *anim = new QSvgAnimateColor(parent);
    anim->appendProperty(prop);

    if (!parseBaseAnimate(parent, attributes, anim, handler)) {
        delete anim;
        return nullptr;
    }

    return anim;
}

static QSvgNode *createAnimateMotionNode(QSvgNode *parent,
                                        const QXmlStreamAttributes &attributes,
                                        QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return nullptr;
}

static void parseNumberTriplet(QList<qreal> &values, QStringView *s)
{
    QList<qreal> list = parseNumbersList(s);
    values << list;
    for (int i = 3 - list.size(); i > 0; --i)
        values.append(0.0);
}

static void parseNumberTriplet(QList<qreal> &values, QStringView s)
{
    parseNumberTriplet(values, &s);
}

QSvgNode *createAnimateTransformNode(QSvgNode *parent,
                                     const QXmlStreamAttributes &attributes,
                                     QSvgHandler *handler)
{
    const QStringView typeStr = attributes.value(QLatin1String("type"));
    const QStringView values  = attributes.value(QLatin1String("values"));
    const QStringView fromStr = attributes.value(QLatin1String("from"));
    const QStringView toStr   = attributes.value(QLatin1String("to"));
    const QStringView byStr   = attributes.value(QLatin1String("by"));

    QList<qreal> vals;
    if (values.isEmpty()) {
        if (fromStr.isEmpty()) {
            if (!byStr.isEmpty()) {
                vals.append(0.0);
                vals.append(0.0);
                vals.append(0.0);
                parseNumberTriplet(vals, byStr);
            } else {
                // To-animation not defined.
                return nullptr;
            }
        } else {
            if (!toStr.isEmpty()) {
                // From-to-animation.
                parseNumberTriplet(vals, fromStr);
                parseNumberTriplet(vals, toStr);
            } else if (!byStr.isEmpty()) {
                // From-by-animation.
                parseNumberTriplet(vals, fromStr);
                parseNumberTriplet(vals, byStr);
                for (int i = vals.size() - 3; i < vals.size(); ++i)
                    vals[i] += vals[i - 3];
            } else {
                return nullptr;
            }
        }
    } else {
        QStringView s = values;
        while (!s.isEmpty()) {
            parseNumberTriplet(vals, &s);
            if (!s.isEmpty())
                s.slice(1);
        }
    }
    if (vals.size() % 3 != 0)
        return nullptr;


    QList<QSvgAnimatedPropertyTransform::TransformComponent> components;
    for (int i = 0; i <= vals.size() - 3; i += 3) {
        QSvgAnimatedPropertyTransform::TransformComponent component;
        if (typeStr == QLatin1String("translate")) {
            component.type = QSvgAnimatedPropertyTransform::TransformComponent::Translate;
            component.values.append(vals.at(i));
            component.values.append(vals.at(i + 1));
        } else if (typeStr == QLatin1String("scale")) {
            component.type = QSvgAnimatedPropertyTransform::TransformComponent::Scale;
            component.values.append(vals.at(i));
            component.values.append(vals.at(i + 1));
        } else if (typeStr == QLatin1String("rotate")) {
            component.type = QSvgAnimatedPropertyTransform::TransformComponent::Rotate;
            component.values.append(vals.at(i));
            component.values.append(vals.at(i + 1));
            component.values.append(vals.at(i + 2));
        } else if (typeStr == QLatin1String("skewX")) {
            component.type = QSvgAnimatedPropertyTransform::TransformComponent::Skew;
            component.values.append(vals.at(i));
            component.values.append(0);
        } else if (typeStr == QLatin1String("skewY")) {
            component.type = QSvgAnimatedPropertyTransform::TransformComponent::Skew;
            component.values.append(0);
            component.values.append(vals.at(i));
        } else {
            return nullptr;
        }
        components.append(component);
    }

    QSvgAnimatedPropertyTransform *prop = static_cast<QSvgAnimatedPropertyTransform *>
        (QSvgAbstractAnimatedProperty::createAnimatedProperty(QLatin1String("transform")));
    if (!prop)
        return nullptr;

    prop->appendComponents(components);
    // <animateTransform> always has one component per key frame
    prop->setTransformCount(1);
    QList<qreal> keyFrames;
    generateKeyFrames(keyFrames, vals.size() / 3);
    prop->setKeyFrames(keyFrames);

    QSvgAnimateTransform *anim = new QSvgAnimateTransform(parent);
    anim->appendProperty(prop);

    if (!parseBaseAnimate(parent, attributes, anim, handler)) {
        delete anim;
        return nullptr;
    }

    return anim;
}

static QSvgNode *createAnimateNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return nullptr;
}

static bool parseAudioNode(QSvgNode *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgNode *createCircleNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    const QStringView cx = attributes.value(QLatin1String("cx"));
    const QStringView cy = attributes.value(QLatin1String("cy"));
    const QStringView r = attributes.value(QLatin1String("r"));
    qreal ncx = QGuiSvg::toDouble(cx);
    qreal ncy = QGuiSvg::toDouble(cy);
    qreal nr  = QGuiSvg::toDouble(r);
    if (nr < 0.0)
        return nullptr;

    QRectF rect(ncx-nr, ncy-nr, nr*2, nr*2);
    QSvgNode *circle = new QSvgCircle(parent, rect);
    return circle;
}

static QSvgNode *createDefsNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *)
{
    Q_UNUSED(attributes);
    QSvgDefs *defs = new QSvgDefs(parent);
    return defs;
}

static bool parseDiscardNode(QSvgNode *parent,
                             const QXmlStreamAttributes &attributes,
                             QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgNode *createEllipseNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    const QStringView cx = attributes.value(QLatin1String("cx"));
    const QStringView cy = attributes.value(QLatin1String("cy"));
    const QStringView rx = attributes.value(QLatin1String("rx"));
    const QStringView ry = attributes.value(QLatin1String("ry"));
    qreal ncx = QGuiSvg::toDouble(cx);
    qreal ncy = QGuiSvg::toDouble(cy);
    qreal nrx = QGuiSvg::toDouble(rx);
    qreal nry = QGuiSvg::toDouble(ry);

    QRectF rect(ncx-nrx, ncy-nry, nrx*2, nry*2);
    QSvgNode *ellipse = new QSvgEllipse(parent, rect);
    return ellipse;
}

static QSvgFontPtr createFontNode(const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    const QStringView hax = attributes.value(QLatin1String("horiz-adv-x"));

    qreal horizAdvX = QGuiSvg::toDouble(hax);

    return std::make_unique<QSvgFont>(horizAdvX);
}

static bool parseFontFaceNode(QSvgFont *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *handler)
{
    const QStringView name = attributes.value(QLatin1String("font-family"));

    if (name.isEmpty())
         return false;

    const QStringView unitsPerEmStr = attributes.value(QLatin1String("units-per-em"));

    /*TODO: Fix toDouble and use the ok flag for testing instead because 0 is a valid
     *      value for unitsPerEm. "units-per-em: <number>" as per definition
     */
    bool ok = false;
    qreal unitsPerEm = QGuiSvg::toDouble(unitsPerEmStr, &ok);
    if (!qFuzzyIsNull(unitsPerEm))
        parent->setUnitsPerEm(unitsPerEm);

    handler->setCurrentSvgFontFamily(name);

    return true;
}

static bool parseFontFaceNameNode(QSvgFont *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseFontFaceSrcNode(QSvgFont *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseFontFaceUriNode(QSvgFont *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseGlyphNode(QSvgFont *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *)
{
    return createSvgGlyph(parent, attributes, false);
}

static bool parseMissingGlyphNode(QSvgFont *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    return createSvgGlyph(parent, attributes, true);
}

static bool parseForeignObjectNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgNode *createGNode(QSvgNode *parent,
                             const QXmlStreamAttributes &attributes,
                             QSvgHandler *)
{
    Q_UNUSED(attributes);
    QSvgG *node = new QSvgG(parent);
    return node;
}

static bool parseHandlerNode(QSvgNode *parent,
                             const QXmlStreamAttributes &attributes,
                             QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseHkernNode(QSvgNode *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgNode *createImageNode(QSvgNode *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *handler)
{
    const QStringView x      = attributes.value(QLatin1String("x"));
    const QStringView y      = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    QString filename         = attributes.value(QLatin1String("xlink:href")).toString();
    if (filename.isEmpty() && !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
        filename = attributes.value(QLatin1String("href")).toString();
    qreal nx = QGuiSvg::toDouble(x);
    qreal ny = QGuiSvg::toDouble(y);
    QGuiSvg::LengthType type;
    qreal nwidth = QGuiSvg::parseLength(width.toString(), &type);
    nwidth = QGuiSvg::convertToPixels(nwidth, true, type);

    qreal nheight = QGuiSvg::parseLength(height.toString(), &type);
    nheight = QGuiSvg::convertToPixels(nheight, false, type);

    filename = filename.trimmed();
    if (filename.isEmpty()) {
        qCWarning(lcSvgHandler) << "QSvgHandler: Image filename is empty";
        return 0;
    }
    if (nwidth <= 0 || nheight <= 0) {
        qCWarning(lcSvgHandler) << "QSvgHandler: Width or height for" << filename << "image was not greater than 0";
        return 0;
    }

    QImage image;
    enum {
        NotLoaded,
        LoadedFromData,
        LoadedFromFile
    } filenameType = NotLoaded;

    if (filename.startsWith(QLatin1String("data"))) {
        QString mimeType;
        QByteArray data;
        if (qDecodeDataUrl(QUrl{filename}, mimeType, data)) {
            image = QImage::fromData(data);
            filenameType = LoadedFromData;
        }
    }

    if (image.isNull()) {
        const auto *file = qobject_cast<QFile *>(handler->device());
        if (file) {
            QUrl url(filename);
            if (url.isRelative()) {
                QFileInfo info(file->fileName());
                filename = info.absoluteDir().absoluteFilePath(filename);
            }
        }

        if (handler->trustedSourceMode() || !QImageReader::imageFormat(filename).startsWith("svg")) {
            image = QImage(filename);
            filenameType = LoadedFromFile;
        }
    }

    if (image.isNull()) {
        qCWarning(lcSvgHandler) << "Could not create image from" << filename;
        return 0;
    }

    if (image.format() == QImage::Format_ARGB32)
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    if (filenameType != LoadedFromFile)
        filename = QString();
    QSvgNode *img = new QSvgImage(parent,
                                  image,
                                  filename,
                                  QRectF(nx,
                                         ny,
                                         nwidth,
                                         nheight));
    return img;
}

static QSvgNode *createLineNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *)
{
    const QStringView x1 = attributes.value(QLatin1String("x1"));
    const QStringView y1 = attributes.value(QLatin1String("y1"));
    const QStringView x2 = attributes.value(QLatin1String("x2"));
    const QStringView y2 = attributes.value(QLatin1String("y2"));
    qreal nx1 = QGuiSvg::toDouble(x1);
    qreal ny1 = QGuiSvg::toDouble(y1);
    qreal nx2 = QGuiSvg::toDouble(x2);
    qreal ny2 = QGuiSvg::toDouble(y2);

    QLineF lineBounds(nx1, ny1, nx2, ny2);
    QSvgNode *line = new QSvgLine(parent, lineBounds);
    return line;
}


static void parseBaseGradient(const QXmlStreamAttributes &attributes,
                              QSvgGradientPaint *gradProp,
                              QSvgHandler *handler)
{
    QStringView linkId                 = attributes.value(QLatin1String("xlink:href"));
    const QStringView trans            = attributes.value(QLatin1String("gradientTransform"));
    const QStringView spread           = attributes.value(QLatin1String("spreadMethod"));
    const QStringView units            = attributes.value(QLatin1String("gradientUnits"));
    const QStringView colorStr         = attributes.value(QLatin1String("color"));
    const QStringView colorOpacityStr  = attributes.value(QLatin1String("color-opacity"));

    QColor color;
    if (constructColor(colorStr, colorOpacityStr, color, handler)) {
        handler->popColor();
        handler->pushColor(color);
    }

    QTransform matrix;
    QGradient *grad = gradProp->qgradient();
    linkId = idFromIRI(linkId);

    if (!linkId.isEmpty()) {
        QSvgPaintServerSharedPtr paintServer = handler->document()->paintServer(linkId);
        if (paintServer && paintServer->type() == QSvgPaintServer::Type::Gradient) {
            QSvgGradientPaint *inherited =
                static_cast<QSvgGradientPaint*>(paintServer.get());
            if (!inherited->stopLink().isEmpty()) {
                gradProp->setStopLink(inherited->stopLink(), handler->document());
            } else {
                grad->setStops(inherited->qgradient()->stops());
                gradProp->setGradientStopsSet(inherited->gradientStopsSet());
            }

            matrix = inherited->qtransform();
        } else {
            gradProp->setStopLink(linkId.toString(), handler->document());
        }
    }

    if (!trans.isEmpty()) {
        matrix = parseTransformationMatrix(trans);
        gradProp->setTransform(matrix);
    } else if (!matrix.isIdentity()) {
        gradProp->setTransform(matrix);
    }

    if (!spread.isEmpty()) {
        if (spread == QLatin1String("pad")) {
            grad->setSpread(QGradient::PadSpread);
        } else if (spread == QLatin1String("reflect")) {
            grad->setSpread(QGradient::ReflectSpread);
        } else if (spread == QLatin1String("repeat")) {
            grad->setSpread(QGradient::RepeatSpread);
        }
    }

    if (units.isEmpty() || units == QLatin1String("objectBoundingBox")) {
         grad->setCoordinateMode(QGradient::ObjectMode);
    }
}


static QSvgPaintServerSharedPtr createLinearGradientNode(const QXmlStreamAttributes &attributes,
                                                         QSvgHandler *handler)
{
    const QStringView x1 = attributes.value(QLatin1String("x1"));
    const QStringView y1 = attributes.value(QLatin1String("y1"));
    const QStringView x2 = attributes.value(QLatin1String("x2"));
    const QStringView y2 = attributes.value(QLatin1String("y2"));

    qreal nx1 = 0.0;
    qreal ny1 = 0.0;
    qreal nx2 = 1.0;
    qreal ny2 = 0.0;

    if (!x1.isEmpty())
        nx1 =  convertToNumber(x1);
    if (!y1.isEmpty())
        ny1 =  convertToNumber(y1);
    if (!x2.isEmpty())
        nx2 =  convertToNumber(x2);
    if (!y2.isEmpty())
        ny2 =  convertToNumber(y2);

    auto grad = std::make_unique<QLinearGradient>(nx1, ny1, nx2, ny2);
    grad->setInterpolationMode(QGradient::ComponentInterpolation);

    QSvgGradientPaintSharedPtr paintServer = std::make_shared<QSvgGradientPaint>(std::move(grad));
    parseBaseGradient(attributes, paintServer.get(), handler);

    return paintServer;
}

static bool parseMetadataNode(QSvgNode *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseMpathNode(QSvgNode *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseMaskNode(QSvgNode *parent,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseMarkerNode(QSvgNode *,
                          const QXmlStreamAttributes &,
                          QSvgHandler *)
{
    return true;
}

static QSvgNode *createMaskNode(QSvgNode *parent,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *handler)
{
    const QStringView x      = attributes.value(QLatin1String("x"));
    const QStringView y      = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    const QStringView mU     = attributes.value(QLatin1String("maskUnits"));
    const QStringView mCU    = attributes.value(QLatin1String("maskContentUnits"));

    QtSvg::UnitTypes nmU = mU.contains(QLatin1String("userSpaceOnUse")) ?
                QtSvg::UnitTypes::userSpaceOnUse : QtSvg::UnitTypes::objectBoundingBox;

    QtSvg::UnitTypes nmCU = mCU.contains(QLatin1String("objectBoundingBox")) ?
                QtSvg::UnitTypes::objectBoundingBox : QtSvg::UnitTypes::userSpaceOnUse;

    bool ok;
    QGuiSvg::LengthType type;

    QtSvg::UnitTypes nmUx = nmU;
    QtSvg::UnitTypes nmUy = nmU;
    QtSvg::UnitTypes nmUw = nmU;
    QtSvg::UnitTypes nmUh = nmU;
    qreal nx = QGuiSvg::parseLength(x, &type, &ok);
    nx = QGuiSvg::convertToPixels(nx, true, type);
    if (x.isEmpty() || !ok) {
        nx = -0.1;
        nmUx = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QGuiSvg::LengthType::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nx = nx / 100. * handler->document()->viewBox().width();
    } else if (type == QGuiSvg::LengthType::LT_PERCENT) {
        nx = nx / 100.;
    }

    qreal ny = QGuiSvg::parseLength(y, &type, &ok);
    ny = QGuiSvg::convertToPixels(ny, true, type);
    if (y.isEmpty() || !ok) {
        ny = -0.1;
        nmUy = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QGuiSvg::LengthType::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        ny = ny / 100. * handler->document()->viewBox().height();
    } else if (type == QGuiSvg::LengthType::LT_PERCENT) {
        ny = ny / 100.;
    }

    qreal nwidth = QGuiSvg::parseLength(width, &type, &ok);
    nwidth = QGuiSvg::convertToPixels(nwidth, true, type);
    if (width.isEmpty() || !ok) {
        nwidth = 1.2;
        nmUw = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QGuiSvg::LengthType::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nwidth = nwidth / 100. * handler->document()->viewBox().width();
    } else if (type == QGuiSvg::LengthType::LT_PERCENT) {
        nwidth = nwidth / 100.;
    }

    qreal nheight = QGuiSvg::parseLength(height, &type, &ok);
    nheight = QGuiSvg::convertToPixels(nheight, true, type);
    if (height.isEmpty() || !ok) {
        nheight = 1.2;
        nmUh = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QGuiSvg::LengthType::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nheight = nheight / 100. * handler->document()->viewBox().height();
    } else if (type == QGuiSvg::LengthType::LT_PERCENT) {
        nheight = nheight / 100.;
    }

    QRectF bounds(nx, ny, nwidth, nheight);
    if (bounds.isEmpty())
        return nullptr;

    QSvgNode *mask = new QSvgMask(parent, QSvgRectF(bounds, nmUx, nmUy, nmUw, nmUh), nmCU);

    return mask;
}

static void parseFilterBounds(const QXmlStreamAttributes &attributes, QSvgRectF *rect)
{
    const QStringView xStr        = attributes.value(QLatin1String("x"));
    const QStringView yStr        = attributes.value(QLatin1String("y"));
    const QStringView widthStr    = attributes.value(QLatin1String("width"));
    const QStringView heightStr   = attributes.value(QLatin1String("height"));

    qreal x = 0;
    if (!xStr.isEmpty()) {
        QGuiSvg::LengthType type;
        x = QGuiSvg::parseLength(xStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT) {
            x = QGuiSvg::convertToPixels(x, true, type);
            rect->setUnitX(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QGuiSvg::LengthType::LT_PERCENT) {
            x /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setX(x);
    }
    qreal y = 0;
    if (!yStr.isEmpty()) {
        QGuiSvg::LengthType type;
        y = QGuiSvg::parseLength(yStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT) {
            y = QGuiSvg::convertToPixels(y, false, type);
            rect->setUnitY(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QGuiSvg::LengthType::LT_PERCENT) {
            y /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setY(y);
    }
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        QGuiSvg::LengthType type;
        width = QGuiSvg::parseLength(widthStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT) {
            width = QGuiSvg::convertToPixels(width, true, type);
            rect->setUnitW(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QGuiSvg::LengthType::LT_PERCENT) {
            width /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setWidth(width);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        QGuiSvg::LengthType type;
        height = QGuiSvg::parseLength(heightStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT) {
            height = QGuiSvg::convertToPixels(height, false, type);
            rect->setUnitH(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QGuiSvg::LengthType::LT_PERCENT) {
            height /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setHeight(height);
    }
}

static QSvgNode *createFilterNode(QSvgNode *parent,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *handler)
{
    const QStringView fU = attributes.value(QLatin1String("filterUnits"));
    const QStringView pU = attributes.value(QLatin1String("primitiveUnits"));

    const QtSvg::UnitTypes filterUnits = fU.contains(QLatin1String("userSpaceOnUse")) ?
                QtSvg::UnitTypes::userSpaceOnUse : QtSvg::UnitTypes::objectBoundingBox;

    const QtSvg::UnitTypes primitiveUnits = pU.contains(QLatin1String("objectBoundingBox")) ?
                QtSvg::UnitTypes::objectBoundingBox : QtSvg::UnitTypes::userSpaceOnUse;

    // https://www.w3.org/TR/SVG11/filters.html#FilterEffectsRegion
    // If ‘x’ or ‘y’ is not specified, the effect is as if a value of -10% were specified.
    // If ‘width’ or ‘height’ is not specified, the effect is as if a value of 120% were specified.
    QSvgRectF rect;
    if (filterUnits == QtSvg::UnitTypes::userSpaceOnUse) {
        qreal width = handler->document()->viewBox().width();
        qreal height = handler->document()->viewBox().height();
        rect = QSvgRectF(QRectF(-0.1 * width, -0.1 * height, 1.2 * width, 1.2 * height),
                         QtSvg::UnitTypes::userSpaceOnUse, QtSvg::UnitTypes::userSpaceOnUse,
                         QtSvg::UnitTypes::userSpaceOnUse, QtSvg::UnitTypes::userSpaceOnUse);
    } else {
        rect = QSvgRectF(QRectF(-0.1, -0.1, 1.2, 1.2),
                         QtSvg::UnitTypes::objectBoundingBox, QtSvg::UnitTypes::objectBoundingBox,
                         QtSvg::UnitTypes::objectBoundingBox, QtSvg::UnitTypes::objectBoundingBox);
    }

    parseFilterBounds(attributes, &rect);

    QSvgNode *filter = new QSvgFilterContainer(parent, rect, filterUnits, primitiveUnits);
    return filter;
}

static void parseFilterAttributes(const QXmlStreamAttributes &attributes, QString *inString,
                                  QString *outString, QSvgRectF *rect)
{
    *inString = attributes.value(QLatin1String("in")).toString();
    *outString = attributes.value(QLatin1String("result")).toString();

    // https://www.w3.org/TR/SVG11/filters.html#FilterPrimitiveSubRegion
    // the default subregion is 0%,0%,100%,100%, where as a special-case the percentages are
    // relative to the dimensions of the filter region, thus making the the default filter primitive
    // subregion equal to the filter region.
    *rect = QSvgRectF(QRectF(0, 0, 1.0, 1.0),
                      QtSvg::UnitTypes::unknown, QtSvg::UnitTypes::unknown,
                      QtSvg::UnitTypes::unknown, QtSvg::UnitTypes::unknown);
    // if we recognize unit == unknown we use the filter as a reference instead of the item, see
    // QSvgFeFilterPrimitive::localSubRegion

    parseFilterBounds(attributes, rect);
}

static QSvgNode *createFeColorMatrixNode(QSvgNode *parent,
                                        const QXmlStreamAttributes &attributes,
                                        QSvgHandler *)
{
    const QStringView typeString   = attributes.value(QLatin1String("type"));
    const QStringView valuesString = attributes.value(QLatin1String("values"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    QSvgFeColorMatrix::ColorShiftType type;
    QSvgFeColorMatrix::Matrix values;
    values.fill(0);

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    if (typeString.startsWith(QLatin1String("saturate")))
        type = QSvgFeColorMatrix::ColorShiftType::Saturate;
    else if (typeString.startsWith(QLatin1String("hueRotate")))
        type = QSvgFeColorMatrix::ColorShiftType::HueRotate;
    else if (typeString.startsWith(QLatin1String("luminanceToAlpha")))
        type = QSvgFeColorMatrix::ColorShiftType::LuminanceToAlpha;
    else
        type = QSvgFeColorMatrix::ColorShiftType::Matrix;

    if (!valuesString.isEmpty()) {
        const auto valueStringList = splitWithDelimiter(valuesString);
        for (int i = 0, j = 0; i < qMin(20, valueStringList.size()); i++) {
            bool ok;
            qreal v = QGuiSvg::toDouble(valueStringList.at(i), &ok);
            if (ok) {
                values.data()[j] = v;
                j++;
            }
        }
    } else {
        values.setToIdentity();
    }

    QSvgNode *filter = new QSvgFeColorMatrix(parent, inputString, outputString, rect,
                                             type, values);
    return filter;
}

static QSvgNode *createFeGaussianBlurNode(QSvgNode *parent,
                                          const QXmlStreamAttributes &attributes,
                                          QSvgHandler *)
{
    const QStringView edgeModeString     = attributes.value(QLatin1String("edgeMode"));
    const QStringView stdDeviationString = attributes.value(QLatin1String("stdDeviation"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    QSvgFeGaussianBlur::EdgeMode edgemode = QSvgFeGaussianBlur::EdgeMode::Duplicate;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);
    qreal stdDeviationX = 0;
    qreal stdDeviationY = 0;
    if (stdDeviationString.contains(QStringLiteral(" "))){
        stdDeviationX = qMax(0., QGuiSvg::toDouble(stdDeviationString.split(u" ").constFirst()));
        stdDeviationY = qMax(0., QGuiSvg::toDouble(stdDeviationString.split(u" ").constLast()));
    } else {
        stdDeviationY = stdDeviationX = qMax(0., QGuiSvg::toDouble(stdDeviationString));
    }

    if (edgeModeString.startsWith(QLatin1String("wrap")))
        edgemode = QSvgFeGaussianBlur::EdgeMode::Wrap;
    else if (edgeModeString.startsWith(QLatin1String("none")))
        edgemode = QSvgFeGaussianBlur::EdgeMode::None;

    QSvgNode *filter = new QSvgFeGaussianBlur(parent, inputString, outputString, rect,
                                              stdDeviationX, stdDeviationY, edgemode);
    return filter;
}

static QSvgNode *createFeOffsetNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &attributes,
                                    QSvgHandler *)
{
    QStringView dxString = attributes.value(QLatin1String("dx"));
    QStringView dyString = attributes.value(QLatin1String("dy"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    qreal dx = 0;
    if (!dxString.isEmpty()) {
        QGuiSvg::LengthType type;
        dx = QGuiSvg::parseLength(dxString, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            dx = QGuiSvg::convertToPixels(dx, true, type);
    }

    qreal dy = 0;
    if (!dyString.isEmpty()) {
        QGuiSvg::LengthType type;
        dy = QGuiSvg::parseLength(dyString, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            dy = QGuiSvg::convertToPixels(dy, true, type);
    }

    QSvgNode *filter = new QSvgFeOffset(parent, inputString, outputString, rect,
                                        dx, dy);
    return filter;
}

static QSvgNode *createFeCompositeNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    const QStringView in2String      = attributes.value(QLatin1String("in2"));
    const QStringView operatorString = attributes.value(QLatin1String("operator"));
    const QStringView k1String       = attributes.value(QLatin1String("k1"));
    const QStringView k2String       = attributes.value(QLatin1String("k2"));
    const QStringView k3String       = attributes.value(QLatin1String("k3"));
    const QStringView k4String       = attributes.value(QLatin1String("k4"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgFeComposite::Operator op = QSvgFeComposite::Operator::Over;
    if (operatorString.startsWith(QLatin1String("in")))
        op = QSvgFeComposite::Operator::In;
    else if (operatorString.startsWith(QLatin1String("out")))
        op = QSvgFeComposite::Operator::Out;
    else if (operatorString.startsWith(QLatin1String("atop")))
        op = QSvgFeComposite::Operator::Atop;
    else if (operatorString.startsWith(QLatin1String("xor")))
        op = QSvgFeComposite::Operator::Xor;
    else if (operatorString.startsWith(QLatin1String("lighter")))
        op = QSvgFeComposite::Operator::Lighter;
    else if (operatorString.startsWith(QLatin1String("arithmetic")))
        op = QSvgFeComposite::Operator::Arithmetic;

    QVector4D k(0, 0, 0, 0);

    if (op == QSvgFeComposite::Operator::Arithmetic) {
        bool ok;
        qreal v = QGuiSvg::toDouble(k1String, &ok);
        if (ok)
            k.setX(v);
        v = QGuiSvg::toDouble(k2String, &ok);
        if (ok)
            k.setY(v);
        v = QGuiSvg::toDouble(k3String, &ok);
        if (ok)
            k.setZ(v);
        v = QGuiSvg::toDouble(k4String, &ok);
        if (ok)
            k.setW(v);
    }

    QSvgNode *filter = new QSvgFeComposite(parent, inputString, outputString, rect,
                                           in2String.toString(), op, k);
    return filter;
}


static QSvgNode *createFeMergeNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeMerge(parent, inputString, outputString, rect);
    return filter;
}

static QSvgNode *createFeFloodNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *handler)
{
    QStringView colorStr          = attributes.value(QLatin1String("flood-color"));
    const QStringView opacityStr  = attributes.value(QLatin1String("flood-opacity"));

    QColor color;
    if (!constructColor(colorStr, opacityStr, color, handler)) {
        color = QColor(Qt::black);
        if (opacityStr.isEmpty())
            color.setAlphaF(1.0);
        else
            setAlpha(opacityStr, &color);
    }

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeFlood(parent, inputString, outputString, rect, color);
    return filter;
}

static QSvgNode *createFeMergeNodeNode(QSvgNode *parent,
                                       const QXmlStreamAttributes &attributes,
                                       QSvgHandler *)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeMergeNode(parent, inputString, outputString, rect);
    return filter;
}

static QSvgNode *createFeBlendNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    const QStringView in2String = attributes.value(QLatin1String("in2"));
    const QStringView modeString = attributes.value(QLatin1String("mode"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgFeBlend::Mode mode = QSvgFeBlend::Mode::Normal;
    if (modeString.startsWith(QLatin1StringView("multiply")))
        mode = QSvgFeBlend::Mode::Multiply;
    else if (modeString.startsWith(QLatin1StringView("screen")))
        mode = QSvgFeBlend::Mode::Screen;
    else if (modeString.startsWith(QLatin1StringView("darken")))
        mode = QSvgFeBlend::Mode::Darken;
    else if (modeString.startsWith(QLatin1StringView("lighten")))
        mode = QSvgFeBlend::Mode::Lighten;

    QSvgNode *filter = new QSvgFeBlend(parent, inputString, outputString, rect,
                                       in2String.toString(), mode);
    return filter;
}

static QSvgNode *createFeUnsupportedNode(QSvgNode *parent,
                                         const QXmlStreamAttributes &attributes,
                                         QSvgHandler *)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(attributes, &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeUnsupported(parent, inputString, outputString, rect);
    return filter;
}

static std::optional<QRectF> parseViewBox(QStringView str)
{
    QList<QStringView> viewBoxValues;

    if (!str.isEmpty())
        viewBoxValues = splitWithDelimiter(str);
    if (viewBoxValues.size() == 4) {
        QGuiSvg::LengthType type;
        qreal x = QGuiSvg::parseLength(viewBoxValues.at(0).trimmed(), &type);
        qreal y = QGuiSvg::parseLength(viewBoxValues.at(1).trimmed(), &type);
        qreal w = QGuiSvg::parseLength(viewBoxValues.at(2).trimmed(), &type);
        qreal h = QGuiSvg::parseLength(viewBoxValues.at(3).trimmed(), &type);
        return QRectF(x, y, w, h);
    }
    return std::nullopt;
}

static bool parseSymbolLikeAttributes(const QXmlStreamAttributes &attributes, QSvgHandler *handler,
                                      QRectF *rect, QRectF *viewBox, QPointF *refPoint,
                                      QSvgSymbolLike::PreserveAspectRatios *aspect,
                                      QSvgSymbolLike::Overflow *overflow,
                                      bool marker = false)
{
    const QStringView xStr        = attributes.value(QLatin1String("x"));
    const QStringView yStr        = attributes.value(QLatin1String("y"));
    const QStringView refXStr     = attributes.value(QLatin1String("refX"));
    const QStringView refYStr     = attributes.value(QLatin1String("refY"));
    const QStringView widthStr    = attributes.value(marker ? QLatin1String("markerWidth")
                                                            : QLatin1String("width"));
    const QStringView heightStr   = attributes.value(marker ? QLatin1String("markerHeight")
                                                            : QLatin1String("height"));
    const QStringView pAspectRStr = attributes.value(QLatin1String("preserveAspectRatio"));
    const QStringView overflowStr = attributes.value(QLatin1String("overflow"));
    const QStringView viewBoxStr  = attributes.value(QLatin1String("viewBox"));


    qreal x = 0;
    if (!xStr.isEmpty()) {
        QGuiSvg::LengthType type;
        x = QGuiSvg::parseLength(xStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            x = QGuiSvg::convertToPixels(x, true, type);
    }
    qreal y = 0;
    if (!yStr.isEmpty()) {
        QGuiSvg::LengthType type;
        y = QGuiSvg::parseLength(yStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            y = QGuiSvg::convertToPixels(y, false, type);
    }
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        QGuiSvg::LengthType type;
        width = QGuiSvg::parseLength(widthStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            width = QGuiSvg::convertToPixels(width, true, type);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        QGuiSvg::LengthType type;
        height = QGuiSvg::parseLength(heightStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            height = QGuiSvg::convertToPixels(height, false, type);
    }

    *rect = QRectF(x, y, width, height);

    x = 0;
    if (!refXStr.isEmpty()) {
        QGuiSvg::LengthType type;
        x = QGuiSvg::parseLength(refXStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            x = QGuiSvg::convertToPixels(x, true, type);
    }
    y = 0;
    if (!refYStr.isEmpty()) {
        QGuiSvg::LengthType type;
        y = QGuiSvg::parseLength(refYStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            y = QGuiSvg::convertToPixels(y, false, type);
    }
    *refPoint = QPointF(x,y);

    auto viewBoxResult = parseViewBox(viewBoxStr);
    if (viewBoxResult)
        *viewBox = *viewBoxResult;
    else if (width > 0 && height > 0)
        *viewBox = QRectF(0, 0, width, height);
    else
        *viewBox = handler->document()->viewBox();

    if (viewBox->isNull())
        return false;

    auto pAspectRStrs = pAspectRStr.split(u" ");
    QSvgSymbolLike::PreserveAspectRatio aspectX = QSvgSymbolLike::PreserveAspectRatio::xMid;
    QSvgSymbolLike::PreserveAspectRatio aspectY = QSvgSymbolLike::PreserveAspectRatio::yMid;
    QSvgSymbolLike::PreserveAspectRatio aspectMS = QSvgSymbolLike::PreserveAspectRatio::meet;

    for (auto &pAStr : std::as_const(pAspectRStrs)) {
        if (pAStr.startsWith(QLatin1String("none"))) {
            aspectX = QSvgSymbolLike::PreserveAspectRatio::None;
            aspectY = QSvgSymbolLike::PreserveAspectRatio::None;
        }else {
            if (pAStr.startsWith(QLatin1String("xMin")))
                aspectX = QSvgSymbolLike::PreserveAspectRatio::xMin;
            else if (pAStr.startsWith(QLatin1String("xMax")))
                aspectX = QSvgSymbolLike::PreserveAspectRatio::xMax;
            if (pAStr.endsWith(QLatin1String("YMin")))
                aspectY = QSvgSymbolLike::PreserveAspectRatio::yMin;
            else if (pAStr.endsWith(QLatin1String("YMax")))
                aspectY = QSvgSymbolLike::PreserveAspectRatio::yMax;
        }

        if (pAStr.endsWith(QLatin1String("slice")))
            aspectMS = QSvgSymbolLike::PreserveAspectRatio::slice;
    }
    *aspect = aspectX | aspectY | aspectMS;

    // overflow is not limited to the symbol element but it is often found with the symbol element.
    // the symbol element makes little sense without the overflow attribute so it is added here.
    // if we decide to remove this from QSvgSymbol, the default value should be set to visible.

    // The default value is visible but chrome uses default value hidden.
    *overflow = QSvgSymbolLike::Overflow::Hidden;

    if (overflowStr.endsWith(QLatin1String("auto")))
        *overflow = QSvgSymbolLike::Overflow::Auto;
    else if (overflowStr.endsWith(QLatin1String("visible")))
        *overflow = QSvgSymbolLike::Overflow::Visible;
    else if (overflowStr.endsWith(QLatin1String("hidden")))
        *overflow = QSvgSymbolLike::Overflow::Hidden;
    else if (overflowStr.endsWith(QLatin1String("scroll")))
        *overflow = QSvgSymbolLike::Overflow::Scroll;

    return true;
}

static QSvgNode *createSymbolNode(QSvgNode *parent,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *handler)
{
    QRectF rect, viewBox;
    QPointF refP;
    QSvgSymbolLike::PreserveAspectRatios aspect;
    QSvgSymbolLike::Overflow overflow;

    if (!parseSymbolLikeAttributes(attributes, handler, &rect, &viewBox, &refP, &aspect, &overflow))
        return nullptr;

    refP = QPointF(0, 0); //refX, refY is ignored in Symbol in Firefox and Chrome.
    QSvgNode *symbol = new QSvgSymbol(parent, rect, viewBox, refP, aspect, overflow);
    return symbol;
}

static QSvgNode *createMarkerNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *handler)
{
    QRectF rect, viewBox;
    QPointF refP;
    QSvgSymbolLike::PreserveAspectRatios aspect;
    QSvgSymbolLike::Overflow overflow;

    const QStringView orientStr      = attributes.value(QLatin1String("orient"));
    const QStringView markerUnitsStr = attributes.value(QLatin1String("markerUnits"));

    qreal orientationAngle = 0;
    QSvgMarker::Orientation orientation;
    if (orientStr.startsWith(QLatin1String("auto-start-reverse")))
        orientation = QSvgMarker::Orientation::AutoStartReverse;
    else if (orientStr.startsWith(QLatin1String("auto")))
        orientation = QSvgMarker::Orientation::Auto;
    else {
        orientation = QSvgMarker::Orientation::Value;
        bool ok;
        qreal a;
        if (orientStr.endsWith(QLatin1String("turn")))
            a = 360. * QGuiSvg::toDouble(orientStr.mid(0, orientStr.length()-4), &ok);
        else if (orientStr.endsWith(QLatin1String("grad")))
            a = QGuiSvg::toDouble(orientStr.mid(0, orientStr.length()-4), &ok);
        else if (orientStr.endsWith(QLatin1String("rad")))
            a = 180. / M_PI * QGuiSvg::toDouble(orientStr.mid(0, orientStr.length()-3), &ok);
        else
            a = QGuiSvg::toDouble(orientStr, &ok);
        if (ok)
            orientationAngle = a;
    }

    QSvgMarker::MarkerUnits markerUnits = QSvgMarker::MarkerUnits::StrokeWidth;
    if (markerUnitsStr.startsWith(QLatin1String("userSpaceOnUse")))
        markerUnits = QSvgMarker::MarkerUnits::UserSpaceOnUse;

    if (!parseSymbolLikeAttributes(attributes, handler, &rect, &viewBox, &refP, &aspect, &overflow, true))
        return nullptr;

    QSvgNode *marker = new QSvgMarker(parent, rect, viewBox, refP, aspect, overflow,
                                      orientation, orientationAngle, markerUnits);
    return marker;
}

static QSvgNode *createPathNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *handler)
{
    QStringView data = attributes.value(QLatin1String("d"));

    std::optional<QPainterPath> qpath = QGuiSvg::parsePath(data,
                                                !handler->trustedSourceMode());
    if (!qpath) {
        qCWarning(lcSvgHandler, "Invalid path data; path truncated.");
        return nullptr;
    }

    qpath.value().setFillRule(Qt::WindingFill);
    QSvgNode *path = new QSvgPath(parent, qpath.value());
    return path;
}

static QSvgNode *createPolyNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                bool createLine)
{
    QStringView pointsStr = attributes.value(QLatin1String("points"));
    const QList<qreal> points = parseNumbersList(&pointsStr);
    if (points.size() < 4)
        return nullptr;
    QPolygonF poly(points.size()/2);
    for (int i = 0; i < poly.size(); ++i)
        poly[i] = QPointF(points.at(2 * i), points.at(2 * i + 1));
    if (createLine)
        return new QSvgPolyline(parent, poly);
    else
        return new QSvgPolygon(parent, poly);
}

static QSvgNode *createPolygonNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    return createPolyNode(parent, attributes, false);
}

static QSvgNode *createPolylineNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &attributes,
                                    QSvgHandler *)
{
    return createPolyNode(parent, attributes, true);
}

static bool parsePrefetchNode(QSvgNode *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgPaintServerSharedPtr createRadialGradientNode(const QXmlStreamAttributes &attributes,
                                                         QSvgHandler *handler)
{
    const QStringView cx = attributes.value(QLatin1String("cx"));
    const QStringView cy = attributes.value(QLatin1String("cy"));
    const QStringView r  = attributes.value(QLatin1String("r"));
    const QStringView fx = attributes.value(QLatin1String("fx"));
    const QStringView fy = attributes.value(QLatin1String("fy"));

    qreal ncx = 0.5;
    qreal ncy = 0.5;
    if (!cx.isEmpty())
        ncx = convertToNumber(cx);
    if (!cy.isEmpty())
        ncy = convertToNumber(cy);

    qreal nr = 0.5;
    if (!r.isEmpty())
        nr = convertToNumber(r);
    if (nr <= 0.0)
        return nullptr;

    qreal nfx = ncx;
    if (!fx.isEmpty())
        nfx = convertToNumber(fx);
    qreal nfy = ncy;
    if (!fy.isEmpty())
        nfy = convertToNumber(fy);

    auto grad = std::make_unique<QRadialGradient>(ncx, ncy, nr, nfx, nfy, 0);
    grad->setInterpolationMode(QGradient::ComponentInterpolation);

    QSvgGradientPaintSharedPtr paintServer = std::make_shared<QSvgGradientPaint>(std::move(grad));
    parseBaseGradient(attributes, paintServer.get(), handler);

    return paintServer;
}

static QSvgNode *createRectNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *)
{
    const QStringView x      = attributes.value(QLatin1String("x"));
    const QStringView y      = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    const QStringView rx      = attributes.value(QLatin1String("rx"));
    const QStringView ry      = attributes.value(QLatin1String("ry"));

    bool ok = true;
    QGuiSvg::LengthType type;
    qreal nwidth = QGuiSvg::parseLength(width, &type, &ok);
    if (!ok)
        return nullptr;
    nwidth = QGuiSvg::convertToPixels(nwidth, true, type);
    qreal nheight = QGuiSvg::parseLength(height, &type, &ok);
    if (!ok)
        return nullptr;
    nheight = QGuiSvg::convertToPixels(nheight, true, type);
    qreal nrx = QGuiSvg::toDouble(rx);
    qreal nry = QGuiSvg::toDouble(ry);

    QRectF bounds(QGuiSvg::toDouble(x), QGuiSvg::toDouble(y), nwidth, nheight);
    if (bounds.isEmpty())
        return nullptr;

    if (!rx.isEmpty() && ry.isEmpty())
        nry = nrx;
    else if (!ry.isEmpty() && rx.isEmpty())
        nrx = nry;

    //9.2 The 'rect'  element clearly specifies it
    // but the case might in fact be handled because
    // we draw rounded rectangles differently
    if (nrx > bounds.width()/2)
        nrx = bounds.width()/2;
    if (nry > bounds.height()/2)
        nry = bounds.height()/2;

    //we draw rounded rect from 0...99
    //svg from 0...bounds.width()/2 so we're adjusting the
    //coordinates
    nrx *= (100/(bounds.width()/2));
    nry *= (100/(bounds.height()/2));

    QSvgNode *rect = new QSvgRect(parent, bounds, nrx, nry);
    return rect;
}

static bool parseScriptNode(QSvgNode *parent,
                            const QXmlStreamAttributes &attributes,
                            QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseSetNode(QSvgNode *parent,
                         const QXmlStreamAttributes &attributes,
                         QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgPaintServerSharedPtr createSolidColorNode(const QXmlStreamAttributes &attributes,
                                                     QSvgHandler *handler)
{
    Q_UNUSED(attributes);
    QStringView solidColorStr = attributes.value(QLatin1String("solid-color"));
    QStringView solidOpacityStr = attributes.value(QLatin1String("solid-opacity"));

    if (solidOpacityStr.isEmpty())
        solidOpacityStr = attributes.value(QLatin1String("opacity"));

    QColor color;
    if (!constructColor(solidColorStr, solidOpacityStr, color, handler))
        return 0;
    std::shared_ptr<QSvgSolidColorPaint> paintServer = std::make_shared<QSvgSolidColorPaint>(color);
    return paintServer;
}

static bool parseStopNode(QSvgPaintServer *paintServer,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *handler)
{
    if (paintServer->type() != QSvgPaintServer::Type::Gradient)
        return false;
    QString nodeIdStr     = someId(attributes);
    QString xmlClassStr   = attributes.value(QLatin1String("class")).toString();

    //### nasty hack because stop gradients are not in the rendering tree
    //    we force a dummy node with the same id and class into a rendering
    //    tree to figure out whether the selector has a style for it
    //    QSvgStyleSelector should be coded in a way that could avoid it
    QSvgDummyNode dummy;
    dummy.setNodeId(nodeIdStr);
    dummy.setXmlClass(xmlClassStr);

    QSvgAttributes attrs(attributes, handler);

#ifndef QT_NO_CSSPARSER
    QXmlStreamAttributes cssAttributes;
    handler->cssHandler().styleLookup(&dummy, cssAttributes);
    attrs.setAttributes(cssAttributes, handler);

    QXmlStreamAttributes styleCssAttributes;
    QStringView style = attributes.value(QLatin1String("style"));
    if (!style.isEmpty())
        handler->cssHandler().parseCSStoXMLAttrs(style.toString(), styleCssAttributes);
    attrs.setAttributes(styleCssAttributes, handler);
#endif

    //TODO: Handle style parsing for gradients stop like the rest of the nodes.
    parseColor(&dummy, attrs, handler);

    QSvgGradientPaint *gradientStyle = static_cast<QSvgGradientPaint*>(paintServer);
    QStringView colorStr    = attrs.stopColor;
    QColor color;

    bool ok = true;
    qreal offset = convertToNumber(attrs.offset, &ok);
    if (!ok)
        offset = 0.0;

    if (!constructColor(colorStr, attrs.stopOpacity, color, handler)) {
        color = Qt::black;
        if (!attrs.stopOpacity.isEmpty())
            setAlpha(attrs.stopOpacity, &color);
    }

    QGradient *grad = gradientStyle->qgradient();

    offset = qMin(qreal(1), qMax(qreal(0), offset)); // Clamp to range [0, 1]
    QGradientStops stops;
    if (gradientStyle->gradientStopsSet()) {
        stops = grad->stops();
        // If the stop offset equals the one previously added, add an epsilon to make it greater.
        if (offset <= stops.back().first)
            offset = stops.back().first + FLT_EPSILON;
    }

    // If offset is greater than one, it must be clamped to one.
    if (offset > 1.0) {
        if ((stops.size() == 1) || (stops.at(stops.size() - 2).first < 1.0 - FLT_EPSILON)) {
            stops.back().first = 1.0 - FLT_EPSILON;
            grad->setStops(stops);
        }
        offset = 1.0;
    }

    grad->setColorAt(offset, color);
    gradientStyle->setGradientStopsSet(true);
    return true;
}

static bool parseStyleNode(QSvgNode *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *handler)
{
    Q_UNUSED(parent);
#ifdef QT_NO_CSSPARSER
    Q_UNUSED(attributes);
    Q_UNUSED(handler);
#else
    const QStringView type = attributes.value(QLatin1String("type"));
    if (type.compare(QLatin1String("text/css"), Qt::CaseInsensitive) == 0 || type.isNull())
        handler->setInStyle(true);
#endif

    return true;
}

static QSvgNode *createSvgNode(QSvgNode *parent,
                               const QXmlStreamAttributes &attributes,
                               QSvgHandler *handler)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);

    QSvgDocument *node = new QSvgDocument(handler->options(), handler->animatorType());
    const QStringView widthStr  = attributes.value(QLatin1String("width"));
    const QStringView heightStr = attributes.value(QLatin1String("height"));
    const QStringView viewBoxStr = attributes.value(QLatin1String("viewBox"));

    QGuiSvg::LengthType type = QGuiSvg::LengthType::LT_PX; // FIXME: is the default correct?
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        width = QGuiSvg::parseLength(widthStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            width = QGuiSvg::convertToPixels(width, true, type);
        node->setWidth(int(width), type == QGuiSvg::LengthType::LT_PERCENT);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        height = QGuiSvg::parseLength(heightStr, &type);
        if (type != QGuiSvg::LengthType::LT_PT)
            height = QGuiSvg::convertToPixels(height, false, type);
        node->setHeight(int(height), type == QGuiSvg::LengthType::LT_PERCENT);
    }

    auto viewBoxResult = parseViewBox(viewBoxStr);
    if (viewBoxResult) {
        node->setViewBox(*viewBoxResult);
    } else if (width && height) {
        if (type == QGuiSvg::LengthType::LT_PT) {
            width = QGuiSvg::convertToPixels(width, false, type);
            height = QGuiSvg::convertToPixels(height, false, type);
        }
        node->setViewBox(QRectF(0, 0, width, height));
    }
    handler->setDefaultCoordinateSystem(QGuiSvg::LengthType::LT_PX);

    return node;
}

static QSvgNode *createSwitchNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    Q_UNUSED(attributes);
    QSvgSwitch *node = new QSvgSwitch(parent);
    return node;
}

static QSvgNode *createPatternNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *handler)
{
    const QStringView x      = attributes.value(QLatin1String("x"));
    const QStringView y      = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    const QStringView patternUnits     = attributes.value(QLatin1String("patternUnits"));
    const QStringView patternContentUnits    = attributes.value(QLatin1String("patternContentUnits"));
    const QStringView patternTransform = attributes.value(QLatin1String("patternTransform"));

    QtSvg::UnitTypes nPatternUnits = patternUnits.contains(QLatin1String("userSpaceOnUse")) ?
                                        QtSvg::UnitTypes::userSpaceOnUse : QtSvg::UnitTypes::objectBoundingBox;

    QtSvg::UnitTypes nPatternContentUnits = patternContentUnits.contains(QLatin1String("objectBoundingBox")) ?
                                               QtSvg::UnitTypes::objectBoundingBox : QtSvg::UnitTypes::userSpaceOnUse;

    const QStringView viewBoxStr = attributes.value(QLatin1String("viewBox"));

    bool ok = false;
    QGuiSvg::LengthType type;

    qreal nx = QGuiSvg::parseLength(x, &type, &ok);
    nx = QGuiSvg::convertToPixels(nx, true, type);
    if (!ok)
        nx = 0.0;
    else if (type == QGuiSvg::LengthType::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nx = (nx / 100.) * handler->document()->viewBox().width();
    else if (type == QGuiSvg::LengthType::LT_PERCENT)
        nx = nx / 100.;

    qreal ny = QGuiSvg::parseLength(y, &type, &ok);
    ny = QGuiSvg::convertToPixels(ny, true, type);
    if (!ok)
        ny = 0.0;
    else if (type == QGuiSvg::LengthType::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        ny = (ny / 100.) * handler->document()->viewBox().height();
    else if (type == QGuiSvg::LengthType::LT_PERCENT)
        ny = ny / 100.;

    qreal nwidth = QGuiSvg::parseLength(width, &type, &ok);
    nwidth = QGuiSvg::convertToPixels(nwidth, true, type);
    if (!ok)
        nwidth = 0.0;
    else if (type == QGuiSvg::LengthType::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nwidth = (nwidth / 100.) * handler->document()->viewBox().width();
    else if (type == QGuiSvg::LengthType::LT_PERCENT)
        nwidth = nwidth / 100.;

    qreal nheight = QGuiSvg::parseLength(height, &type, &ok);
    nheight = QGuiSvg::convertToPixels(nheight, true, type);
    if (!ok)
        nheight = 0.0;
    else if (type == QGuiSvg::LengthType::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nheight = (nheight / 100.) * handler->document()->viewBox().height();
    else if (type == QGuiSvg::LengthType::LT_PERCENT)
        nheight = nheight / 100.;

    QRectF viewBox;
    auto viewBoxResult = parseViewBox(viewBoxStr);
    if (viewBoxResult) {
        if (viewBoxResult->width() > 0 && viewBoxResult->height() > 0)
            viewBox = *viewBoxResult;
    }

    QTransform matrix;
    if (!patternTransform.isEmpty())
        matrix = parseTransformationMatrix(patternTransform);

    QRectF bounds(nx, ny, nwidth, nheight);
    if (bounds.isEmpty())
        return nullptr;

    QSvgRectF patternRectF(bounds, nPatternUnits, nPatternUnits, nPatternUnits, nPatternUnits);
    QSvgPattern *node = new QSvgPattern(parent, patternRectF, viewBox, nPatternContentUnits, matrix);

    // Create a style node for the Pattern.
    QSvgPaintServerSharedPtr prop = std::make_shared<QSvgPatternPaint>(node);
    handler->document()->addPaintServer(std::move(prop), someId(attributes));

    return node;
}

static bool parseTbreakNode(QSvgNode *parent,
                            const QXmlStreamAttributes &,
                            QSvgHandler *)
{
    if (parent->type() != QSvgNode::Textarea)
        return false;
    static_cast<QSvgText*>(parent)->addLineBreak();
    return true;
}

static QSvgNode *createTextNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *)
{
    const QStringView x = attributes.value(QLatin1String("x"));
    const QStringView y = attributes.value(QLatin1String("y"));
    //### editable and rotate not handled
    QGuiSvg::LengthType type;
    qreal nx = QGuiSvg::parseLength(x, &type);
    nx = QGuiSvg::convertToPixels(nx, true, type);
    qreal ny = QGuiSvg::parseLength(y, &type);
    ny = QGuiSvg::convertToPixels(ny, true, type);

    QSvgNode *text = new QSvgText(parent, QPointF(nx, ny));
    return text;
}

static QSvgNode *createTextAreaNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &attributes,
                                    QSvgHandler *handler)
{
    QSvgText *node = static_cast<QSvgText *>(createTextNode(parent, attributes, handler));
    if (node) {
        QGuiSvg::LengthType type;
        qreal width = QGuiSvg::parseLength(attributes.value(QLatin1String("width")), &type);
        qreal height = QGuiSvg::parseLength(attributes.value(QLatin1String("height")), &type);
        node->setTextArea(QSizeF(width, height));
    }
    return node;
}

static QSvgNode *createTspanNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &,
                                    QSvgHandler *)
{
    return new QSvgTspan(parent);
}

static QSvgNode *createUseNode(QSvgNode *parent,
                               const QXmlStreamAttributes &attributes,
                               QSvgHandler *handler)
{
    QStringView linkId     = attributes.value(QLatin1String("xlink:href"));
    const QStringView xStr = attributes.value(QLatin1String("x"));
    const QStringView yStr = attributes.value(QLatin1String("y"));

    if (linkId.isEmpty())
        linkId = attributes.value(QLatin1String("href"));
    QString linkIdStr = idFromIRI(linkId).toString();

    switch (parent->type()) {
    case QSvgNode::Doc:
    case QSvgNode::Defs:
    case QSvgNode::Group:
    case QSvgNode::Switch:
    case QSvgNode::Mask:
    case QSvgNode::Symbol:
    case QSvgNode::Marker:
    case QSvgNode::Pattern:
        break;
    default:
        qCWarning(lcSvgHandler, "<use> element %ls in wrong context!", qUtf16Printable(linkIdStr));
        return 0;
    }

    QPointF pt;
    if (!xStr.isNull() || !yStr.isNull()) {
        QGuiSvg::LengthType type;
        qreal nx = QGuiSvg::parseLength(xStr, &type);
        nx = QGuiSvg::convertToPixels(nx, true, type);

        qreal ny = QGuiSvg::parseLength(yStr, &type);
        ny = QGuiSvg::convertToPixels(ny, true, type);
        pt = QPointF(nx, ny);
    }

    QSvgNode *link = handler->document()->namedNode(linkIdStr);
    if (link) {
        if (parent->isDescendantOf(link))
            qCWarning(lcSvgHandler, "link %ls is recursive!", qUtf16Printable(linkIdStr));

        return new QSvgUse(pt, parent, link);
    }

    //delay link resolving, link might have not been created yet
    return new QSvgUse(pt, parent, linkIdStr);
}

static QSvgNode *createVideoNode(QSvgNode *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return 0;
}

typedef QSvgNode *(*FactoryMethod)(QSvgNode *, const QXmlStreamAttributes &, QSvgHandler *);

static FactoryMethod findGroupFactory(const QStringView name, QtSvg::Options options)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = name.mid(1);
    switch (name.at(0).unicode()) {
    case 'd':
        if (ref == QLatin1String("efs")) return createDefsNode;
        break;
    case 'f':
        if (ref == QLatin1String("ilter") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return createFilterNode;
        break;
    case 'g':
        if (ref.isEmpty()) return createGNode;
        break;
    case 'm':
        if (ref == QLatin1String("ask") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return createMaskNode;
        if (ref == QLatin1String("arker") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return createMarkerNode;
        break;
    case 's':
        if (ref == QLatin1String("vg")) return createSvgNode;
        if (ref == QLatin1String("witch")) return createSwitchNode;
        if (ref == QLatin1String("ymbol") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return createSymbolNode;
        break;
    case 'p':
        if (ref == QLatin1String("attern") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return createPatternNode;
        break;
    default:
        break;
    }
    return 0;
}

static FactoryMethod findGraphicsFactory(const QStringView name, QtSvg::Options options)
{
    Q_UNUSED(options);
    if (name.isEmpty())
        return 0;

    QStringView ref = name.mid(1);
    switch (name.at(0).unicode()) {
    case 'c':
        if (ref == QLatin1String("ircle")) return createCircleNode;
        break;
    case 'e':
        if (ref == QLatin1String("llipse")) return createEllipseNode;
        break;
    case 'i':
        if (ref == QLatin1String("mage")) return createImageNode;
        break;
    case 'l':
        if (ref == QLatin1String("ine")) return createLineNode;
        break;
    case 'p':
        if (ref == QLatin1String("ath")) return createPathNode;
        if (ref == QLatin1String("olygon")) return createPolygonNode;
        if (ref == QLatin1String("olyline")) return createPolylineNode;
        break;
    case 'r':
        if (ref == QLatin1String("ect")) return createRectNode;
        break;
    case 't':
        if (ref == QLatin1String("ext")) return createTextNode;
        if (ref == QLatin1String("extArea")) return createTextAreaNode;
        if (ref == QLatin1String("span")) return createTspanNode;
        break;
    case 'u':
        if (ref == QLatin1String("se")) return createUseNode;
        break;
    case 'v':
        if (ref == QLatin1String("ideo")) return createVideoNode;
        break;
    default:
        break;
    }
    return 0;
}

static FactoryMethod findFilterFactory(const QStringView name, QtSvg::Options options)
{
    if (options.testFlag(QtSvg::Tiny12FeaturesOnly))
        return 0;

    if (name.isEmpty())
        return 0;

    if (!name.startsWith(QLatin1String("fe")))
        return 0;

    if (name == QLatin1String("feMerge")) return createFeMergeNode;
    if (name == QLatin1String("feColorMatrix")) return createFeColorMatrixNode;
    if (name == QLatin1String("feGaussianBlur")) return createFeGaussianBlurNode;
    if (name == QLatin1String("feOffset")) return createFeOffsetNode;
    if (name == QLatin1String("feMergeNode")) return createFeMergeNodeNode;
    if (name == QLatin1String("feComposite")) return createFeCompositeNode;
    if (name == QLatin1String("feFlood")) return createFeFloodNode;
    if (name == QLatin1String("feBlend")) return createFeBlendNode;

    static const QStringList unsupportedFilters = {
        QStringLiteral("feComponentTransfer"),
        QStringLiteral("feConvolveMatrix"),
        QStringLiteral("feDiffuseLighting"),
        QStringLiteral("feDisplacementMap"),
        QStringLiteral("feDropShadow"),
        QStringLiteral("feFuncA"),
        QStringLiteral("feFuncB"),
        QStringLiteral("feFuncG"),
        QStringLiteral("feFuncR"),
        QStringLiteral("feImage"),
        QStringLiteral("feMorphology"),
        QStringLiteral("feSpecularLighting"),
        QStringLiteral("feTile"),
        QStringLiteral("feTurbulence")
    };

    if (unsupportedFilters.contains(name))
        return createFeUnsupportedNode;

    return 0;
}

typedef QSvgNode *(*AnimationMethod)(QSvgNode *, const QXmlStreamAttributes &, QSvgHandler *);

static AnimationMethod findAnimationFactory(const QStringView name, QtSvg::Options options)
{
    if (name.isEmpty() || options.testFlag(QtSvg::DisableSMILAnimations))
        return 0;

    QStringView ref = name.mid(1);

    switch (name.at(0).unicode()) {
    case 'a':
        if (ref == QLatin1String("nimate")) return createAnimateNode;
        if (ref == QLatin1String("nimateColor")) return createAnimateColorNode;
        if (ref == QLatin1String("nimateMotion")) return createAnimateMotionNode;
        if (ref == QLatin1String("nimateTransform")) return createAnimateTransformNode;
        break;
    default:
        break;
    }

    return 0;
}

typedef bool (*ParseMethod)(QSvgNode *, const QXmlStreamAttributes &, QSvgHandler *);

static ParseMethod findUtilFactory(const QStringView name, QtSvg::Options options)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = name.mid(1);
    switch (name.at(0).unicode()) {
    case 'a':
        if (ref.isEmpty()) return parseAnchorNode;
        if (ref == QLatin1String("udio")) return parseAudioNode;
        break;
    case 'd':
        if (ref == QLatin1String("iscard")) return parseDiscardNode;
        break;
    case 'f':
        if (ref == QLatin1String("oreignObject")) return parseForeignObjectNode;
        break;
    case 'h':
        if (ref == QLatin1String("andler")) return parseHandlerNode;
        if (ref == QLatin1String("kern")) return parseHkernNode;
        break;
    case 'm':
        if (ref == QLatin1String("etadata")) return parseMetadataNode;
        if (ref == QLatin1String("path")) return parseMpathNode;
        if (ref == QLatin1String("ask") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return parseMaskNode;
        if (ref == QLatin1String("arker") && !options.testFlag(QtSvg::Tiny12FeaturesOnly)) return parseMarkerNode;
        break;
    case 'p':
        if (ref == QLatin1String("refetch")) return parsePrefetchNode;
        break;
    case 's':
        if (ref == QLatin1String("cript")) return parseScriptNode;
        if (ref == QLatin1String("et")) return parseSetNode;
        if (ref == QLatin1String("tyle")) return parseStyleNode;
        break;
    case 't':
        if (ref == QLatin1String("break")) return parseTbreakNode;
        break;
    default:
        break;
    }
    return 0;
}

typedef QSvgFontPtr (*FontFactoryMethod)(const QXmlStreamAttributes &,
                                         QSvgHandler *);

static FontFactoryMethod findFontFactoryMethod(const QStringView name)
{
    if (name == "font"_L1)
        return createFontNode;

    return nullptr;
}

typedef bool (*FontParseMethod)(QSvgFont *,
                                 const QXmlStreamAttributes &,
                                 QSvgHandler *);

static FontParseMethod findFontParseFactoryMethod(const QStringView name)
{
    if (name.isEmpty())
        return nullptr;

    QStringView ref = name.mid(1);
    switch (name.at(0).unicode()) {
    case 'f':
        if (ref == QLatin1String("ont-face")) return parseFontFaceNode;
        if (ref == QLatin1String("ont-face-name")) return parseFontFaceNameNode;
        if (ref == QLatin1String("ont-face-src")) return parseFontFaceSrcNode;
        if (ref == QLatin1String("ont-face-uri")) return parseFontFaceUriNode;
        break;
    case 'g':
        if (ref == QLatin1String("lyph")) return parseGlyphNode;
        break;
    case 'm':
        if (ref == QLatin1String("issing-glyph")) return parseMissingGlyphNode;
        break;
    default:
        break;
    }
    return nullptr;
}

typedef QSvgPaintServerSharedPtr (*PaintServerFactoryMethod)(const QXmlStreamAttributes &,
                                                                     QSvgHandler *);

static PaintServerFactoryMethod findPaintServerFactoryMethod(const QStringView name)
{
    if (name.isEmpty())
        return nullptr;

    QStringView ref = name.sliced(1);
    switch (name.at(0).unicode()) {
    case 'l':
        if (ref == QLatin1String("inearGradient")) return createLinearGradientNode;
        break;
    case 'r':
        if (ref == QLatin1String("adialGradient")) return createRadialGradientNode;
        break;
    case 's':
        if (ref == QLatin1String("olidColor")) return createSolidColorNode;
        break;
    default:
        break;
    }
    return nullptr;
}

typedef bool (*PaintServerParseMethod)(QSvgPaintServer *,
                                 const QXmlStreamAttributes &,
                                 QSvgHandler *);

static PaintServerParseMethod findPaintServerUtilFactoryMethod(const QStringView name)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = name.sliced(1);
    switch (name.at(0).unicode()) {
    case 's':
        if (ref == QLatin1String("top")) return parseStopNode;
        break;
    default:
        break;
    }
    return 0;
}

QSvgHandler::QSvgHandler(QIODevice *device, QtSvg::Options options,
                         QtSvg::AnimatorType type)
    : xml(new QXmlStreamReader(device))
    , m_ownsReader(true)
    , m_options(options)
    , m_animatorType(type)
{
    init();
}

QSvgHandler::QSvgHandler(const QByteArray &data, QtSvg::Options options,
                         QtSvg::AnimatorType type)
    : xml(new QXmlStreamReader(data))
    , m_ownsReader(true)
    , m_options(options)
    , m_animatorType(type)
{
    init();
}

QSvgHandler::QSvgHandler(QXmlStreamReader *const reader, QtSvg::Options options,
                         QtSvg::AnimatorType type)
    : xml(reader)
    , m_ownsReader(false)
    , m_options(options)
    , m_animatorType(type)
{
    init();
}

void QSvgHandler::init()
{
    m_animEnd = 0;
    m_defaultCoords = QGuiSvg::LT_PX;
    m_defaultPen = QPen(Qt::black, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
    m_defaultPen.setMiterLimit(4);
    parse();
}

static bool detectPatternCycles(const QSvgNode *node, QList<const QSvgNode *> &linkable)
{
    const QSvgFillStyle *fillStyle = static_cast<const QSvgFillStyle*>
        (node->styleProperty(QSvgStyleProperty::Fill));
    if (fillStyle && fillStyle->paintServer()
        && fillStyle->paintServer()->type() == QSvgPaintServer::Type::Pattern) {
        QSvgPatternPaint *patternStyle = static_cast<QSvgPatternPaint *>(fillStyle->paintServer());
        if (linkable.contains(patternStyle->patternNode()))
            return true;
    }

    const QSvgStrokeStyle *strokeStyle = static_cast<const QSvgStrokeStyle*>
        (node->styleProperty(QSvgStyleProperty::Stroke));
    if (strokeStyle && strokeStyle->paintServer()
        && strokeStyle->paintServer()->type() == QSvgPaintServer::Type::Pattern) {
        QSvgPatternPaint *patternStyle = static_cast<QSvgPatternPaint *>(strokeStyle->paintServer());
        if (linkable.contains(patternStyle->patternNode()))
            return true;
    }

    return false;
}

/* The function goes through a node and its descendants to
 * find any circular references in the parsed SVG file. It
 * is important for this to happen non-recursively to avoid
 * stack overflows.
 * The function maintains two lists of nodes. The list "linkable"
 * is used to track patterns and uses because these are the nodes
 * that can be referenced by other nodes.
 * Example :
 * <pattern id="pat1" />
 *  <rect fill="url(#pat1)" />
 * </pattern>
 *
 * The other list of nodes is a stack to traverse the tree
 * non-recursively, the std::pair stored in the stack will
 * indicate whether a pattern or use has been visited and
 * added to the "linkable" list or not. If the bool is set to true,
 * this element can be popped out from the "linkable" list. */
static bool detectCycles(const QSvgNode *n)
{
    if (Q_UNLIKELY(!n))
        return false;

    QList<const QSvgNode *> linkable;
    using NodeState = std::pair<const QSvgNode *, bool>;
    QStack<NodeState> nodes;
    nodes.push({n, false});

    do {
        auto current = nodes.pop();
        if (current.second) {
            Q_ASSERT(!linkable.isEmpty() && current.first == linkable.back());
            linkable.pop_back();
            continue;
        }

        switch (current.first->type()) {
        case QSvgNode::Doc:
        case QSvgNode::Group:
        case QSvgNode::Defs:
        case QSvgNode::Pattern:
        {
            if (current.first->type() == QSvgNode::Pattern) {
                linkable.append(current.first);
                nodes.push({current.first, true});
            }
            auto *g = static_cast<const QSvgStructureNode*>(current.first);
            for (auto it = g->renderers().crbegin(); it != g->renderers().crend(); it++)
                nodes.push({it->get(), false});
        }
        break;
        case QSvgNode::Use:
        {
            if (linkable.contains(current.first))
                return true;
            auto *u = static_cast<const QSvgUse*>(current.first);
            auto *target = u->link();
            if (target) {
                linkable.append(u);
                nodes.push({u, true});
                nodes.push({target, false});
            }
        }
        break;
        case QSvgNode::Rect:
        case QSvgNode::Ellipse:
        case QSvgNode::Circle:
        case QSvgNode::Line:
        case QSvgNode::Path:
        case QSvgNode::Polygon:
        case QSvgNode::Polyline:
        case QSvgNode::Tspan:
            if (detectPatternCycles(current.first, linkable))
                return true;
            break;
        default:
            break;
        }
    } while (!nodes.isEmpty());
    return false;
}

static bool detectCyclesAndWarn(const QSvgNode *node) {
    const bool cycleFound = detectCycles(node);
    if (cycleFound)
        qCWarning(lcSvgHandler, "Cycles detected in SVG, document discarded.");
    return cycleFound;
}

// Having too many unfinished elements will cause a stack overflow
// in the dtor of QSvgDocument, see oss-fuzz issue 24000.
static const int unfinishedElementsLimit = 2048;

void QSvgHandler::parse()
{
    xml->setNamespaceProcessing(false);
#ifndef QT_NO_CSSPARSER
    m_inStyle = false;
#endif
    bool done = false;
    int remainingUnfinishedElements = unfinishedElementsLimit;
    while (!xml->atEnd() && !done) {
        switch (xml->readNext()) {
        case QXmlStreamReader::StartElement:
            // he we could/should verify the namespaces, and simply
            // call m_skipNodes(Unknown) if we don't know the
            // namespace.  We do support http://www.w3.org/2000/svg
            // but also http://www.w3.org/2000/svg-20000303-stylable
            // And if the document uses an external dtd, the reported
            // namespaceUri is empty. The only possible strategy at
            // this point is to do what everyone else seems to do and
            // ignore the reported namespaceUri completely.
            if (remainingUnfinishedElements && startElement(xml->name(), xml->attributes())) {
                --remainingUnfinishedElements;
            } else {
                m_doc.reset();
                return;
            }
            break;
        case QXmlStreamReader::EndElement:
            done = endElement(xml->name());
            ++remainingUnfinishedElements;
            break;
        case QXmlStreamReader::Characters:
            characters(xml->text());
            break;
        case QXmlStreamReader::ProcessingInstruction:
            processingInstruction(xml->processingInstructionTarget(), xml->processingInstructionData());
            break;
        default:
            break;
        }
    }

    if (!m_doc)
        return;

    resolvePaintServers();
    resolveNodes();
    if (detectCyclesAndWarn(m_doc.get()))
        m_doc.reset();
}

bool QSvgHandler::startElement(const QStringView localName,
                               const QXmlStreamAttributes &attributes)
{
    QSvgNode *node = nullptr;

    pushColorCopy();

    /* The xml:space attribute may appear on any element. We do
     * a lookup by the qualified name here, but this is namespace aware, since
     * the XML namespace can only be bound to prefix "xml." */
    const QStringView xmlSpace(attributes.value(QLatin1String("xml:space")));
    if (xmlSpace.isNull()) {
        // This element has no xml:space attribute.
        m_whitespaceMode.push(m_whitespaceMode.isEmpty() ? QSvgText::Default : m_whitespaceMode.top());
    } else if (xmlSpace == QLatin1String("preserve")) {
        m_whitespaceMode.push(QSvgText::Preserve);
    } else if (xmlSpace == QLatin1String("default")) {
        m_whitespaceMode.push(QSvgText::Default);
    } else {
        const QByteArray msg = '"' + xmlSpace.toLocal8Bit()
                               + "\" is an invalid value for attribute xml:space. "
                                 "Valid values are \"preserve\" and \"default\".";
        qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
        m_whitespaceMode.push(QSvgText::Default);
    }

    if (!m_doc && localName != QLatin1String("svg"))
        return false;

    if (m_doc && localName == QLatin1String("svg")) {
        m_skipNodes.push(Doc);
        qCWarning(lcSvgHandler) << "Skipping a nested svg element, because "
                                   "SVG Document must not contain nested svg elements in Svg Tiny 1.2";
    }

    if (!m_skipNodes.isEmpty() && m_skipNodes.top() == Doc)
        return true;

    if (FactoryMethod method = findGroupFactory(localName, options())) {
        //group
        if (!m_doc) {
            node = method(nullptr, attributes, this);
            if (node) {
                Q_ASSERT(node->type() == QSvgNode::Doc);
                m_doc.reset(static_cast<QSvgDocument*>(node));
            }
        } else {
            switch (m_nodes.top()->type()) {
            case QSvgNode::Doc:
            case QSvgNode::Group:
            case QSvgNode::Defs:
            case QSvgNode::Switch:
            case QSvgNode::Mask:
            case QSvgNode::Symbol:
            case QSvgNode::Marker:
            case QSvgNode::Pattern:
            {
                node = method(m_nodes.top(), attributes, this);
                if (node) {
                    QSvgStructureNode *group =
                        static_cast<QSvgStructureNode*>(m_nodes.top());
                    group->addChild(std::unique_ptr<QSvgNode>(node), someId(attributes));
                }
            }
                break;
            default:
                const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
                qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                break;
            }
        }

        if (node) {
            parseCoreNode(node, attributes);
            parseStyle(node, attributes, this);
            if (node->type() == QSvgNode::Filter)
                m_toBeResolved.append(node);
        }
    } else if (FactoryMethod method = findGraphicsFactory(localName, options())) {
        //rendering element
        Q_ASSERT(!m_nodes.isEmpty());
        switch (m_nodes.top()->type()) {
        case QSvgNode::Doc:
        case QSvgNode::Group:
        case QSvgNode::Defs:
        case QSvgNode::Switch:
        case QSvgNode::Mask:
        case QSvgNode::Symbol:
        case QSvgNode::Marker:
        case QSvgNode::Pattern:
        {
            if (localName == QLatin1String("tspan")) {
                const QByteArray msg = QByteArrayLiteral("\'tspan\' element in wrong context.");
                qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                break;
            }
            node = method(m_nodes.top(), attributes, this);
            if (node) {
                QSvgStructureNode *group =
                    static_cast<QSvgStructureNode*>(m_nodes.top());
                group->addChild(std::unique_ptr<QSvgNode>(node), someId(attributes));
            }
        }
            break;
        case QSvgNode::Text:
        case QSvgNode::Textarea:
            if (localName == QLatin1String("tspan")) {
                node = method(m_nodes.top(), attributes, this);
                if (node) {
                    static_cast<QSvgText *>(m_nodes.top())->addTspan(static_cast<QSvgTspan *>(node));
                }
            } else {
                const QByteArray msg = QByteArrayLiteral("\'text\' or \'textArea\' element contains invalid element type.");
                qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
            }
            break;
        default:
            const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
            qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
            break;
        }

        if (node) {
            parseCoreNode(node, attributes);
            parseStyle(node, attributes, this);
            if (node->type() == QSvgNode::Text || node->type() == QSvgNode::Textarea) {
                static_cast<QSvgText *>(node)->setWhitespaceMode(m_whitespaceMode.top());
            } else if (node->type() == QSvgNode::Tspan) {
                static_cast<QSvgTspan *>(node)->setWhitespaceMode(m_whitespaceMode.top());
            } else if (node->type() == QSvgNode::Use) {
                auto useNode = static_cast<QSvgUse *>(node);
                if (!useNode->isResolved())
                    m_toBeResolved.append(useNode);
            }
        }
    } else if (FactoryMethod method = findFilterFactory(localName, options())) {
        //filter nodes to be aded to be filtercontainer
        Q_ASSERT(!m_nodes.isEmpty());
        if (m_nodes.top()->type() == QSvgNode::Filter ||
            (m_nodes.top()->type() == QSvgNode::FeMerge && localName == QLatin1String("feMergeNode"))) {
            node = method(m_nodes.top(), attributes, this);
            if (node) {
                QSvgStructureNode *container =
                    static_cast<QSvgStructureNode*>(m_nodes.top());
                container->addChild(std::unique_ptr<QSvgNode>(node), someId(attributes));
            }
        } else {
            const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
            qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
        }
    } else if (AnimationMethod method = findAnimationFactory(localName, options())) {
        Q_ASSERT(!m_nodes.isEmpty());
        node = method(m_nodes.top(), attributes, this);
        if (node) {
            QSvgAnimateNode *anim = static_cast<QSvgAnimateNode *>(node);
            if (anim->linkId().isEmpty())
                m_doc->animator()->appendAnimation(m_nodes.top(), anim);
            else if (m_doc->namedNode(anim->linkId()))
                m_doc->animator()->appendAnimation(m_doc->namedNode(anim->linkId()), anim);
            else
                m_toBeResolved.append(anim);
        }
    } else if (ParseMethod method = findUtilFactory(localName, options())) {
        Q_ASSERT(!m_nodes.isEmpty());
        if (!method(m_nodes.top(), attributes, this))
            qCWarning(lcSvgHandler, "%s", msgProblemParsing(localName, xml).constData());
    } else if (FontFactoryMethod method = findFontFactoryMethod(localName)) {
        QSvgFontPtr font = method(attributes, this);
        if (font) {
            m_currentSvgFontData.font = std::move(font);
        } else {
            const QByteArray msg = QByteArrayLiteral("Could not parse node: ") + localName.toLocal8Bit();
            qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
        }
    } else if (PaintServerFactoryMethod method = findPaintServerFactoryMethod(localName)) {
        QSvgPaintServerSharedPtr paintServer = method(attributes, this);
        if (paintServer) {
            m_paintServer = paintServer;
            m_doc->addPaintServer(std::move(paintServer), someId(attributes));
        } else {
            const QByteArray msg = QByteArrayLiteral("Could not parse node: ") + localName.toLocal8Bit();
            qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
        }
    } else if (FontParseMethod method = findFontParseFactoryMethod(localName)) {
        if (m_currentSvgFontData.font) {
            if (!method(m_currentSvgFontData.font.get(), attributes, this))
                qCWarning(lcSvgHandler, "%s", msgProblemParsing(localName, xml).constData());
        }
    } else if (PaintServerParseMethod method = findPaintServerUtilFactoryMethod(localName)) {
        if (m_paintServer) {
            if (!method(m_paintServer.get(), attributes, this))
                qCWarning(lcSvgHandler, "%s", msgProblemParsing(localName, xml).constData());
        }
    } else {
        qCDebug(lcSvgHandler) << "Skipping unknown element" << localName;
        m_skipNodes.push(Unknown);
        return true;
    }

    if (node) {
        m_nodes.push(node);
        m_skipNodes.push(Graphics);
    } else {
        //qDebug()<<"Skipping "<<localName;
        m_skipNodes.push(Style);
    }
    return true;
}

bool QSvgHandler::endElement(const QStringView localName)
{
    CurrentNode node = m_skipNodes.top();

    if (node == Doc && localName != QLatin1String("svg"))
        return false;

    m_skipNodes.pop();
    m_whitespaceMode.pop();

    popColor();

    if (node == Unknown)
        return false;

#ifdef QT_NO_CSSPARSER
    Q_UNUSED(localName);
#else
    if (m_inStyle && localName == QLatin1String("style"))
        m_inStyle = false;
#endif

    if (node == Graphics)
        m_nodes.pop();

    if (localName == "font"_L1) {
        if (!m_currentSvgFontData.isEmpty()) {
            document()->addSvgFont(m_currentSvgFontData.fontFamily,
                                   std::move(m_currentSvgFontData.font));
        }
        m_currentSvgFontData.reset();
    }

    return ((localName == QLatin1String("svg")) && (node != Doc));
}

void QSvgHandler::resolvePaintServers()
{
    for (QSvgStyleProperty *prop : std::as_const(m_unresolvedStyles)) {
        if (prop->type() == QSvgStyleProperty::Fill) {
            QSvgFillStyle *fill = static_cast<QSvgFillStyle *>(prop);
            QString id = fill->paintStyleId();
            QSvgPaintServerSharedPtr paintServer = m_doc->paintServer(id);
            if (paintServer) {
                fill->setPaintServer(std::move(paintServer));
            } else {
                qCWarning(lcSvgHandler, "%s", msgCouldNotResolveProperty(id, xml).constData());
                fill->setBrush(Qt::NoBrush);
            }
        } else if (prop->type() == QSvgStyleProperty::Stroke) {
            QSvgStrokeStyle *stroke = static_cast<QSvgStrokeStyle *>(prop);
            QString id = stroke->paintStyleId();
            QSvgPaintServerSharedPtr paintServer = m_doc->paintServer(id);
            if (paintServer) {
                stroke->setPaintServer(std::move(paintServer));
            } else {
                qCWarning(lcSvgHandler, "%s", msgCouldNotResolveProperty(id, xml).constData());
                stroke->setStroke(Qt::NoBrush);
            }
        }
    }

    m_unresolvedStyles.clear();
}

void QSvgHandler::resolveNodes()
{
    for (QSvgNode *node : std::as_const(m_toBeResolved)) {
        if (node->type() == QSvgNode::Use) {
            QSvgUse *useNode = static_cast<QSvgUse *>(node);
            const auto parent = useNode->parent();
            if (!parent)
                continue;

            QSvgNode::Type t = parent->type();
            if (t != QSvgNode::Doc && t != QSvgNode::Defs && t != QSvgNode::Group && t != QSvgNode::Switch)
                continue;

            QSvgNode *link = m_doc->namedNode(useNode->linkId());
            if (!link) {
                qCWarning(lcSvgHandler, "link #%s is undefined!", qPrintable(useNode->linkId()));
                continue;
            }

            if (useNode->parent()->isDescendantOf(link))
                qCWarning(lcSvgHandler, "link #%s is recursive!", qPrintable(useNode->linkId()));

            useNode->setLink(link);
        } else if (node->type() == QSvgNode::Filter) {
            QSvgFilterContainer *filter = static_cast<QSvgFilterContainer *>(node);
            for (auto &renderer : filter->renderers()) {
                const QSvgFeFilterPrimitive *primitive = QSvgFeFilterPrimitive::castToFilterPrimitive(renderer.get());
                if (!primitive || primitive->type() == QSvgNode::FeUnsupported) {
                    filter->setSupported(false);
                    break;
                }
            }
        } else if (node->type() == QSvgNode::AnimateTransform || node->type() == QSvgNode::AnimateColor) {
            QSvgAnimateNode *anim = static_cast<QSvgAnimateNode *>(node);
            QSvgNode *targetNode = m_doc->namedNode(anim->linkId());
            if (targetNode) {
                m_doc->animator()->appendAnimation(targetNode, anim);
            } else {
                qCWarning(lcSvgHandler, "Cannot find target for link #%s!",
                          qPrintable(anim->linkId()));
                delete anim;
            }
        }
    }
    m_toBeResolved.clear();
}

bool QSvgHandler::characters(const QStringView str)
{
#ifndef QT_NO_CSSPARSER
    if (m_inStyle) {
        m_cssHandler.parseStyleSheet(str);
        return true;
    }
#endif
    if (m_skipNodes.isEmpty() || m_skipNodes.top() == Unknown || m_nodes.isEmpty())
        return true;

    if (m_nodes.top()->type() == QSvgNode::Text || m_nodes.top()->type() == QSvgNode::Textarea) {
        static_cast<QSvgText*>(m_nodes.top())->addText(str);
    } else if (m_nodes.top()->type() == QSvgNode::Tspan) {
        static_cast<QSvgTspan*>(m_nodes.top())->addText(str);
    }

    return true;
}

QIODevice *QSvgHandler::device() const
{
    return xml->device();
}

QSvgDocument *QSvgHandler::document() const
{
    return m_doc.get();
}

std::unique_ptr<QSvgDocument> QSvgHandler::takeDocument()
{
    return std::move(m_doc);
}

QGuiSvg::LengthType QSvgHandler::defaultCoordinateSystem() const
{
    return m_defaultCoords;
}

void QSvgHandler::setDefaultCoordinateSystem(QGuiSvg::LengthType type)
{
    m_defaultCoords = type;
}

void QSvgHandler::pushColor(const QColor &color)
{
    m_colorStack.push(color);
    m_colorTagCount.push(1);
}

void QSvgHandler::pushColorCopy()
{
    if (m_colorTagCount.size())
        ++m_colorTagCount.top();
    else
        pushColor(Qt::black);
}

void QSvgHandler::popColor()
{
    if (m_colorTagCount.size()) {
        if (!--m_colorTagCount.top()) {
            m_colorStack.pop();
            m_colorTagCount.pop();
        }
    }
}

QColor QSvgHandler::currentColor() const
{
    if (!m_colorStack.isEmpty())
        return m_colorStack.top();
    else
        return QColor(0, 0, 0);
}

void QSvgHandler::pushUnresolvedStyle(QSvgStyleProperty *prop)
{
    m_unresolvedStyles.append(prop);
}

void QSvgHandler::setCurrentSvgFontFamily(QStringView family)
{
    m_currentSvgFontData.fontFamily = family.toString();
}

#ifndef QT_NO_CSSPARSER

void QSvgHandler::setInStyle(bool b)
{
    m_inStyle = b;
}

bool QSvgHandler::inStyle() const
{
    return m_inStyle;
}

QSvgCssHandler &QSvgHandler::cssHandler()
{
    return m_cssHandler;
}

#endif // QT_NO_CSSPARSER

bool QSvgHandler::processingInstruction(const QStringView target, const QStringView data)
{
#ifdef QT_NO_CSSPARSER
    Q_UNUSED(target);
    Q_UNUSED(data);
#else
    if (target == QLatin1String("xml-stylesheet")) {
        static const QRegularExpression rx(QStringLiteral("type=\\\"(.+)\\\""),
                              QRegularExpression::InvertedGreedinessOption);
        QRegularExpressionMatchIterator iter = rx.globalMatchView(data);
        bool isCss = false;
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            QString type = match.captured(1);
            if (type.toLower() == QLatin1String("text/css")) {
                isCss = true;
            }
        }

        if (isCss) {
            static const QRegularExpression rx(QStringLiteral("href=\\\"(.+)\\\""),
                                  QRegularExpression::InvertedGreedinessOption);
            QRegularExpressionMatch match = rx.matchView(data);
            QString addr = match.captured(1);
            QFileInfo fi(addr);
            //qDebug()<<"External CSS file "<<fi.absoluteFilePath()<<fi.exists();
            if (fi.exists()) {
                QFile file(fi.absoluteFilePath());
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    return true;
                }
                QByteArray cssData = file.readAll();
                QString css = QString::fromUtf8(cssData);
                m_cssHandler.parseStyleSheet(css);
            }

        }
    }
#endif

    return true;
}

void QSvgHandler::setAnimPeriod(int start, int end)
{
    Q_UNUSED(start);
    m_animEnd   = qMax(end, m_animEnd);
}

int QSvgHandler::animationDuration() const
{
    return m_animEnd;
}

QSvgHandler::~QSvgHandler()
{
    if(m_ownsReader)
        delete xml;
}

QT_END_NAMESPACE
