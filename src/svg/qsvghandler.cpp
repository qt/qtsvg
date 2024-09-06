// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"

#include "qsvghandler_p.h"

#include "qsvgtinydocument_p.h"
#include "qsvgstructure_p.h"
#include "qsvggraphics_p.h"
#include "qsvgfilter_p.h"
#include "qsvgnode_p.h"
#include "qsvgfont_p.h"

#include "qpen.h"
#include "qpainterpath.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qtextformat.h"
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
#include "private/qmath_p.h"
#include "qimagereader.h"

#include "float.h"
#include <cmath>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSvgHandler, "qt.svg")

static const char *qt_inherit_text = "inherit";
#define QT_INHERIT QLatin1String(qt_inherit_text)

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

static inline QByteArray msgProblemParsing(const QString &localName, const QXmlStreamReader *r)
{
    return prefixMessage(QByteArrayLiteral("Problem parsing ") + localName.toLocal8Bit(), r);
}

static inline QByteArray msgCouldNotResolveProperty(const QString &id, const QXmlStreamReader *r)
{
    return prefixMessage(QByteArrayLiteral("Could not resolve property: ") + id.toLocal8Bit(), r);
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

static bool parsePathDataFast(QStringView data, QPainterPath &path, bool limitLength = true);

static inline QString someId(const QXmlStreamAttributes &attributes)
{
    QString id = attributes.value(QLatin1String("id")).toString();
    if (id.isEmpty())
        id = attributes.value(QLatin1String("xml:id")).toString();
    return id;
}

struct QSvgAttributes
{
    QSvgAttributes(const QXmlStreamAttributes &xmlAttributes, QSvgHandler *handler);

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


#ifndef QT_NO_CSSPARSER
    QList<QSvgCssAttribute> m_cssAttributes;
#endif
};

QSvgAttributes::QSvgAttributes(const QXmlStreamAttributes &xmlAttributes, QSvgHandler *handler)
{
    for (int i = 0; i < xmlAttributes.size(); ++i) {
        const QXmlStreamAttribute &attribute = xmlAttributes.at(i);
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

    // If a style attribute is present, let its attribute settings override the plain attribute
    // values. The spec seems to indicate that, and it is common behavior in svg renderers.
#ifndef QT_NO_CSSPARSER
    QStringView style = xmlAttributes.value(QLatin1String("style"));
    if (!style.isEmpty()) {
        handler->parseCSStoXMLAttrs(style.toString(), &m_cssAttributes);
        for (int j = 0; j < m_cssAttributes.size(); ++j) {
            const QSvgCssAttribute &attribute = m_cssAttributes.at(j);
            QStringView name = attribute.name;
            QStringView value = attribute.value;
            if (name.isEmpty())
                continue;

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
                if (name == QLatin1String("image-rendering"))
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
                else if (name == QLatin1String("offset"))
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

            default:
                break;
            }
        }
    }
#else
    Q_UNUSED(handler);
#endif // QT_NO_CSSPARSER
}

#ifndef QT_NO_CSSPARSER

class QSvgStyleSelector : public QCss::StyleSelector
{
public:
    QSvgStyleSelector()
    {
        nameCaseSensitivity = Qt::CaseInsensitive;
    }
    virtual ~QSvgStyleSelector()
    {
    }

    inline QString nodeToName(QSvgNode *node) const
    {
        return node->typeName();
    }

    inline QSvgNode *svgNode(NodePtr node) const
    {
        return (QSvgNode*)node.ptr;
    }
    inline QSvgStructureNode *nodeToStructure(QSvgNode *n) const
    {
        if (n &&
            (n->type() == QSvgNode::Doc ||
             n->type() == QSvgNode::Group ||
             n->type() == QSvgNode::Defs ||
             n->type() == QSvgNode::Switch)) {
            return (QSvgStructureNode*)n;
        }
        return 0;
    }

    inline QSvgStructureNode *svgStructure(NodePtr node) const
    {
        QSvgNode *n = svgNode(node);
        QSvgStructureNode *st = nodeToStructure(n);
        return st;
    }

    bool nodeNameEquals(NodePtr node, const QString& nodeName) const override
    {
        QSvgNode *n = svgNode(node);
        if (!n)
            return false;
        QString name = nodeToName(n);
        return QString::compare(name, nodeName, Qt::CaseInsensitive) == 0;
    }
    QString attributeValue(NodePtr node, const QCss::AttributeSelector &asel) const override
    {
        const QString &name = asel.name;
        QSvgNode *n = svgNode(node);
        if ((!n->nodeId().isEmpty() && (name == QLatin1String("id") ||
                                        name == QLatin1String("xml:id"))))
            return n->nodeId();
        if (!n->xmlClass().isEmpty() && name == QLatin1String("class"))
            return n->xmlClass();
        return QString();
    }
    bool hasAttributes(NodePtr node) const override
    {
        QSvgNode *n = svgNode(node);
        return (n &&
                (!n->nodeId().isEmpty() || !n->xmlClass().isEmpty()));
    }

    QStringList nodeIds(NodePtr node) const override
    {
        QSvgNode *n = svgNode(node);
        QString nid;
        if (n)
            nid = n->nodeId();
        QStringList lst; lst.append(nid);
        return lst;
    }

    QStringList nodeNames(NodePtr node) const override
    {
        QSvgNode *n = svgNode(node);
        if (n)
           return QStringList(nodeToName(n));
        return QStringList();
    }

    bool isNullNode(NodePtr node) const override
    {
        return !node.ptr;
    }

    NodePtr parentNode(NodePtr node) const override
    {
        QSvgNode *n = svgNode(node);
        NodePtr newNode;
        newNode.ptr = 0;
        newNode.id = 0;
        if (n) {
            QSvgNode *svgParent = n->parent();
            if (svgParent) {
                newNode.ptr = svgParent;
            }
        }
        return newNode;
    }
    NodePtr previousSiblingNode(NodePtr node) const override
    {
        NodePtr newNode;
        newNode.ptr = 0;
        newNode.id = 0;

        QSvgNode *n = svgNode(node);
        if (!n)
            return newNode;
        QSvgStructureNode *svgParent = nodeToStructure(n->parent());

        if (svgParent) {
            newNode.ptr = svgParent->previousSiblingNode(n);
        }
        return newNode;
    }
    NodePtr duplicateNode(NodePtr node) const override
    {
        NodePtr n;
        n.ptr = node.ptr;
        n.id  = node.id;
        return n;
    }
    void freeNode(NodePtr node) const override
    {
        Q_UNUSED(node);
    }
};

#endif // QT_NO_CSSPARSER

// '0' is 0x30 and '9' is 0x39
static inline bool isDigit(ushort ch)
{
    static quint16 magic = 0x3ff;
    return ((ch >> 4) == 3) && (magic >> (ch & 15));
}

static qreal toDouble(const QChar *&str)
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
        if(*t == '-') {
            neg = true;
            ++t;
        }
        while(*t && *t != '.') {
            ival *= 10;
            ival += (*t) - '0';
            ++t;
        }
        if(*t == '.') {
            ++t;
            int div = 1;
            while(*t) {
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

static qreal toDouble(QStringView str, bool *ok = NULL)
{
    const QChar *c = str.constData();
    qreal res = (c == nullptr ? qreal{} : toDouble(c));
    if (ok)
        *ok = (c == (str.constData() + str.size()));
    return res;
}

static QList<qreal> parseNumbersList(const QChar *&str)
{
    QList<qreal> points;
    if (!str)
        return points;
    points.reserve(32);

    while (str->isSpace())
        ++str;
    while (isDigit(str->unicode()) ||
           *str == QLatin1Char('-') || *str == QLatin1Char('+') ||
           *str == QLatin1Char('.')) {

        points.append(toDouble(str));

        while (str->isSpace())
            ++str;
        if (*str == QLatin1Char(','))
            ++str;

        //eat the rest of space
        while (str->isSpace())
            ++str;
    }

    return points;
}

static inline void parseNumbersArray(const QChar *&str, QVarLengthArray<qreal, 8> &points,
                                     const char *pattern = nullptr)
{
    const size_t patternLen = qstrlen(pattern);
    while (str->isSpace())
        ++str;
    while (isDigit(str->unicode()) ||
           *str == QLatin1Char('-') || *str == QLatin1Char('+') ||
           *str == QLatin1Char('.')) {

        if (patternLen && pattern[points.size() % patternLen] == 'f') {
            // flag expected, may only be 0 or 1
            if (*str != QLatin1Char('0') && *str != QLatin1Char('1'))
                return;
            points.append(*str == QLatin1Char('0') ? 0.0 : 1.0);
            ++str;
        } else {
            points.append(toDouble(str));
        }

        while (str->isSpace())
            ++str;
        if (*str == QLatin1Char(','))
            ++str;

        //eat the rest of space
        while (str->isSpace())
            ++str;
    }
}

static QList<qreal> parsePercentageList(const QChar *&str)
{
    QList<qreal> points;
    if (!str)
        return points;

    while (str->isSpace())
        ++str;
    while ((*str >= QLatin1Char('0') && *str <= QLatin1Char('9')) ||
           *str == QLatin1Char('-') || *str == QLatin1Char('+') ||
           *str == QLatin1Char('.')) {

        points.append(toDouble(str));

        while (str->isSpace())
            ++str;
        if (*str == QLatin1Char('%'))
            ++str;
        while (str->isSpace())
            ++str;
        if (*str == QLatin1Char(','))
            ++str;

        //eat the rest of space
        while (str->isSpace())
            ++str;
    }

    return points;
}

static QString idFromUrl(const QString &url)
{
    // The form is url(<IRI>), where IRI can be
    // just an ID on #<id> form.
    QString::const_iterator itr = url.constBegin();
    QString::const_iterator end = url.constEnd();
    QString id;
    while (itr != end && (*itr).isSpace())
        ++itr;
    if (itr != end && (*itr) == QLatin1Char('('))
        ++itr;
    else
        return QString();
    while (itr != end && (*itr).isSpace())
        ++itr;
    if (itr != end && (*itr) == QLatin1Char('#')) {
        id += *itr;
        ++itr;
    } else {
        return QString();
    }
    while (itr != end && (*itr) != QLatin1Char(')')) {
        id += *itr;
        ++itr;
    }
    if (itr == end || (*itr) != QLatin1Char(')'))
        return QString();
    return id;
}

/**
 * returns true when successfully set the color. false signifies
 * that the color should be inherited
 */
static bool resolveColor(QStringView colorStr, QColor &color, QSvgHandler *handler)
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
                    const QChar *s = colorStrTr.constData() + 4;
                    QList<qreal> compo = parseNumbersList(s);
                    //1 means that it failed after reaching non-parsable
                    //character which is going to be "%"
                    if (compo.size() == 1) {
                        s = colorStrTr.constData() + 4;
                        compo = parsePercentageList(s);
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
            if (colorStrTr == QT_INHERIT)
                return false;
            break;
        default:
            break;
    }

    color = QColor::fromString(colorStrTr.toString());
    return color.isValid();
}

static bool constructColor(QStringView colorStr, QStringView opacity,
                           QColor &color, QSvgHandler *handler)
{
    if (!resolveColor(colorStr, color, handler))
        return false;
    if (!opacity.isEmpty()) {
        bool ok = true;
        qreal op = qMin(qreal(1.0), qMax(qreal(0.0), toDouble(opacity, &ok)));
        if (!ok)
            op = 1.0;
        color.setAlphaF(op);
    }
    return true;
}

static qreal parseLength(QStringView str, QSvgHandler::LengthType *type,
                         QSvgHandler *handler, bool *ok = NULL)
{
    QStringView numStr = str.trimmed();

    if (numStr.isEmpty()) {
        if (ok)
            *ok = false;
        *type = QSvgHandler::LT_OTHER;
        return false;
    }
    if (numStr.endsWith(QLatin1Char('%'))) {
        numStr.chop(1);
        *type = QSvgHandler::LT_PERCENT;
    } else if (numStr.endsWith(QLatin1String("px"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_PX;
    } else if (numStr.endsWith(QLatin1String("pc"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_PC;
    } else if (numStr.endsWith(QLatin1String("pt"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_PT;
    } else if (numStr.endsWith(QLatin1String("mm"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_MM;
    } else if (numStr.endsWith(QLatin1String("cm"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_CM;
    } else if (numStr.endsWith(QLatin1String("in"))) {
        numStr.chop(2);
        *type = QSvgHandler::LT_IN;
    } else {
        *type = handler->defaultCoordinateSystem();
        //type = QSvgHandler::LT_OTHER;
    }
    qreal len = toDouble(numStr, ok);
    //qDebug()<<"len is "<<len<<", from '"<<numStr << "'";
    return len;
}

static inline qreal convertToNumber(QStringView str, QSvgHandler *handler, bool *ok = NULL)
{
    QSvgHandler::LengthType type;
    qreal num = parseLength(str.toString(), &type, handler, ok);
    if (type == QSvgHandler::LT_PERCENT) {
        num = num/100.0;
    }
    return num;
}

static bool createSvgGlyph(QSvgFont *font, const QXmlStreamAttributes &attributes)
{
    QStringView uncStr = attributes.value(QLatin1String("unicode"));
    QStringView havStr = attributes.value(QLatin1String("horiz-adv-x"));
    QStringView pathStr = attributes.value(QLatin1String("d"));

    QChar unicode = (uncStr.isEmpty()) ? u'\0' : uncStr.at(0);
    qreal havx = (havStr.isEmpty()) ? -1 : toDouble(havStr);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    parsePathDataFast(pathStr, path);

    font->addGlyph(unicode, path, havx);

    return true;
}

// this should really be called convertToDefaultCoordinateSystem
// and convert when type != QSvgHandler::defaultCoordinateSystem
static qreal convertToPixels(qreal len, bool , QSvgHandler::LengthType type)
{

    switch (type) {
    case QSvgHandler::LT_PERCENT:
        break;
    case QSvgHandler::LT_PX:
        break;
    case QSvgHandler::LT_PC:
        break;
    case QSvgHandler::LT_PT:
        return len * 1.25;
        break;
    case QSvgHandler::LT_MM:
        return len * 3.543307;
        break;
    case QSvgHandler::LT_CM:
        return len * 35.43307;
        break;
    case QSvgHandler::LT_IN:
        return len * 90;
        break;
    case QSvgHandler::LT_OTHER:
        break;
    default:
        break;
    }
    return len;
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

static QSvgStyleProperty *styleFromUrl(QSvgNode *node, const QString &url)
{
    return node ? node->styleProperty(idFromUrl(url)) : 0;
}

static void parseBrush(QSvgNode *node,
                       const QSvgAttributes &attributes,
                       QSvgHandler *handler)
{
    if (!attributes.fill.isEmpty() || !attributes.fillRule.isEmpty() || !attributes.fillOpacity.isEmpty()) {
        QSvgFillStyle *prop = new QSvgFillStyle;

        //fill-rule attribute handling
        if (!attributes.fillRule.isEmpty() && attributes.fillRule != QT_INHERIT) {
            if (attributes.fillRule == QLatin1String("evenodd"))
                prop->setFillRule(Qt::OddEvenFill);
            else if (attributes.fillRule == QLatin1String("nonzero"))
                prop->setFillRule(Qt::WindingFill);
        }

        //fill-opacity attribute handling
        if (!attributes.fillOpacity.isEmpty() && attributes.fillOpacity != QT_INHERIT) {
            prop->setFillOpacity(qMin(qreal(1.0), qMax(qreal(0.0), toDouble(attributes.fillOpacity))));
        }

        //fill attribute handling
        if ((!attributes.fill.isEmpty()) && (attributes.fill != QT_INHERIT) ) {
            if (attributes.fill.size() > 3 && attributes.fill.mid(0, 3) == QLatin1String("url")) {
                QString value = attributes.fill.mid(3, attributes.fill.size() - 3).toString();
                QSvgStyleProperty *style = styleFromUrl(node, value);
                if (style) {
                    if (style->type() == QSvgStyleProperty::SOLID_COLOR || style->type() == QSvgStyleProperty::GRADIENT
                            || style->type() == QSvgStyleProperty::PATTERN)
                        prop->setFillStyle(reinterpret_cast<QSvgPaintStyleProperty *>(style));
                } else {
                    QString id = idFromUrl(value);
                    prop->setPaintStyleId(id);
                    prop->setPaintStyleResolved(false);
                }
            } else if (attributes.fill != QLatin1String("none")) {
                QColor color;
                if (resolveColor(attributes.fill, color, handler))
                    prop->setBrush(QBrush(color));
            } else {
                prop->setBrush(QBrush(Qt::NoBrush));
            }
        }
        node->appendStyleProperty(prop, attributes.id);
    }
}



static QTransform parseTransformationMatrix(QStringView value)
{
    if (value.isEmpty())
        return QTransform();

    QTransform matrix;
    const QChar *str = value.constData();
    const QChar *end = str + value.size();

    while (str < end) {
        if (str->isSpace() || *str == QLatin1Char(',')) {
            ++str;
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
        if (*str == QLatin1Char('m')) {  //matrix
            const char *ident = "atrix";
            for (int i = 0; i < 5; ++i)
                if (*(++str) != QLatin1Char(ident[i]))
                    goto error;
            ++str;
            state = Matrix;
        } else if (*str == QLatin1Char('t')) { //translate
            const char *ident = "ranslate";
            for (int i = 0; i < 8; ++i)
                if (*(++str) != QLatin1Char(ident[i]))
                    goto error;
            ++str;
            state = Translate;
        } else if (*str == QLatin1Char('r')) { //rotate
            const char *ident = "otate";
            for (int i = 0; i < 5; ++i)
                if (*(++str) != QLatin1Char(ident[i]))
                    goto error;
            ++str;
            state = Rotate;
        } else if (*str == QLatin1Char('s')) { //scale, skewX, skewY
            ++str;
            if (*str == QLatin1Char('c')) {
                const char *ident = "ale";
                for (int i = 0; i < 3; ++i)
                    if (*(++str) != QLatin1Char(ident[i]))
                        goto error;
                ++str;
                state = Scale;
            } else if (*str == QLatin1Char('k')) {
                if (*(++str) != QLatin1Char('e'))
                    goto error;
                if (*(++str) != QLatin1Char('w'))
                    goto error;
                ++str;
                if (*str == QLatin1Char('X'))
                    state = SkewX;
                else if (*str == QLatin1Char('Y'))
                    state = SkewY;
                else
                    goto error;
                ++str;
            } else {
                goto error;
            }
        } else {
            goto error;
        }


        while (str < end && str->isSpace())
            ++str;
        if (*str != QLatin1Char('('))
            goto error;
        ++str;
        QVarLengthArray<qreal, 8> points;
        parseNumbersArray(str, points);
        if (*str != QLatin1Char(')'))
            goto error;
        ++str;

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
    //qDebug()<<"Node "<<node->type()<<", attrs are "<<value<<width;

    if (!attributes.stroke.isEmpty() || !attributes.strokeDashArray.isEmpty() || !attributes.strokeDashOffset.isEmpty() || !attributes.strokeLineCap.isEmpty()
        || !attributes.strokeLineJoin.isEmpty() || !attributes.strokeMiterLimit.isEmpty() || !attributes.strokeOpacity.isEmpty() || !attributes.strokeWidth.isEmpty()
        || !attributes.vectorEffect.isEmpty()) {

        QSvgStrokeStyle *prop = new QSvgStrokeStyle;

        //stroke attribute handling
        if ((!attributes.stroke.isEmpty()) && (attributes.stroke != QT_INHERIT) ) {
            if (attributes.stroke.size() > 3 && attributes.stroke.mid(0, 3) == QLatin1String("url")) {
                 QString value = attributes.stroke.mid(3, attributes.stroke.size() - 3).toString();
                    QSvgStyleProperty *style = styleFromUrl(node, value);
                    if (style) {
                        if (style->type() == QSvgStyleProperty::SOLID_COLOR || style->type() == QSvgStyleProperty::GRADIENT
                            || style->type() == QSvgStyleProperty::PATTERN)
                        prop->setStyle(reinterpret_cast<QSvgPaintStyleProperty *>(style));
                    } else {
                        QString id = idFromUrl(value);
                        prop->setPaintStyleId(id);
                        prop->setPaintStyleResolved(false);
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
        if (!attributes.strokeWidth.isEmpty() && attributes.strokeWidth != QT_INHERIT) {
            QSvgHandler::LengthType lt;
            prop->setWidth(parseLength(attributes.strokeWidth, &lt, handler));
        }

        //stroke-dasharray
        if (!attributes.strokeDashArray.isEmpty() && attributes.strokeDashArray != QT_INHERIT) {
            if (attributes.strokeDashArray == QLatin1String("none")) {
                prop->setDashArrayNone();
            } else {
                QString dashArray  = attributes.strokeDashArray.toString();
                const QChar *s = dashArray.constData();
                QList<qreal> dashes = parseNumbersList(s);
                bool allZeroes = true;
                for (qreal dash : dashes) {
                    if (dash != 0.0) {
                        allZeroes = false;
                        break;
                    }
                }

                // if the stroke dash array contains only zeros,
                // force drawing of solid line.
                if (allZeroes == false) {
                    // if the dash count is odd the dashes should be duplicated
                    if ((dashes.size() & 1) != 0)
                        dashes << QList<qreal>(dashes);
                    prop->setDashArray(dashes);
                } else {
                    prop->setDashArrayNone();
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
        if (!attributes.strokeDashOffset.isEmpty() && attributes.strokeDashOffset != QT_INHERIT)
            prop->setDashOffset(toDouble(attributes.strokeDashOffset));

        //vector-effect attribute handling
        if (!attributes.vectorEffect.isEmpty()) {
            if (attributes.vectorEffect == QLatin1String("non-scaling-stroke"))
                prop->setVectorEffect(true);
            else if (attributes.vectorEffect == QLatin1String("none"))
                prop->setVectorEffect(false);
        }

        //stroke-miterlimit
        if (!attributes.strokeMiterLimit.isEmpty() && attributes.strokeMiterLimit != QT_INHERIT)
            prop->setMiterLimit(toDouble(attributes.strokeMiterLimit));

        //stroke-opacity atttribute handling
        if (!attributes.strokeOpacity.isEmpty() && attributes.strokeOpacity != QT_INHERIT)
            prop->setOpacity(qMin(qreal(1.0), qMax(qreal(0.0), toDouble(attributes.strokeOpacity))));

        node->appendStyleProperty(prop, attributes.id);
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

static void parseFont(QSvgNode *node,
                      const QSvgAttributes &attributes,
                      QSvgHandler *handler)
{
    if (attributes.fontFamily.isEmpty() && attributes.fontSize.isEmpty() && attributes.fontStyle.isEmpty() &&
        attributes.fontWeight.isEmpty() && attributes.fontVariant.isEmpty() && attributes.textAnchor.isEmpty())
        return;

    QSvgFontStyle *fontStyle = nullptr;
    if (!attributes.fontFamily.isEmpty()) {
        QSvgTinyDocument *doc = node->document();
        if (doc) {
            QSvgFont *svgFont = doc->svgFont(attributes.fontFamily.toString());
            if (svgFont)
                fontStyle = new QSvgFontStyle(svgFont, doc);
        }
    }
    if (!fontStyle)
        fontStyle = new QSvgFontStyle;
    if (!attributes.fontFamily.isEmpty() && attributes.fontFamily != QT_INHERIT) {
        QString family = attributes.fontFamily.toString().trimmed();
        if (family.at(0) == QLatin1Char('\'') || family.at(0) == QLatin1Char('\"'))
            family = family.mid(1, family.size() - 2);
        fontStyle->setFamily(family);
    }

    if (!attributes.fontSize.isEmpty() && attributes.fontSize != QT_INHERIT) {
        // TODO: Support relative sizes 'larger' and 'smaller'.
        const FontSizeSpec spec = fontSizeSpec(attributes.fontSize);
        switch (spec) {
        case FontSizeNone:
            break;
        case FontSizeValue: {
            QSvgHandler::LengthType type;
            qreal fs = parseLength(attributes.fontSize, &type, handler);
            fs = convertToPixels(fs, true, type);
            fontStyle->setSize(qMin(fs, qreal(0xffff)));
        }
            break;
        default:
            fontStyle->setSize(sizeTable[spec]);
            break;
        }
    }

    if (!attributes.fontStyle.isEmpty() && attributes.fontStyle != QT_INHERIT) {
        if (attributes.fontStyle == QLatin1String("normal")) {
            fontStyle->setStyle(QFont::StyleNormal);
        } else if (attributes.fontStyle == QLatin1String("italic")) {
            fontStyle->setStyle(QFont::StyleItalic);
        } else if (attributes.fontStyle == QLatin1String("oblique")) {
            fontStyle->setStyle(QFont::StyleOblique);
        }
    }

    if (!attributes.fontWeight.isEmpty() && attributes.fontWeight != QT_INHERIT) {
        bool ok = false;
        const int weightNum = attributes.fontWeight.toInt(&ok);
        if (ok) {
            fontStyle->setWeight(weightNum);
        } else {
            if (attributes.fontWeight == QLatin1String("normal")) {
                fontStyle->setWeight(QFont::Normal);
            } else if (attributes.fontWeight == QLatin1String("bold")) {
                fontStyle->setWeight(QFont::Bold);
            } else if (attributes.fontWeight == QLatin1String("bolder")) {
                fontStyle->setWeight(QSvgFontStyle::BOLDER);
            } else if (attributes.fontWeight == QLatin1String("lighter")) {
                fontStyle->setWeight(QSvgFontStyle::LIGHTER);
            }
        }
    }

    if (!attributes.fontVariant.isEmpty() && attributes.fontVariant != QT_INHERIT) {
        if (attributes.fontVariant == QLatin1String("normal"))
            fontStyle->setVariant(QFont::MixedCase);
        else if (attributes.fontVariant == QLatin1String("small-caps"))
            fontStyle->setVariant(QFont::SmallCaps);
    }

    if (!attributes.textAnchor.isEmpty() && attributes.textAnchor != QT_INHERIT) {
        if (attributes.textAnchor == QLatin1String("start"))
            fontStyle->setTextAnchor(Qt::AlignLeft);
        if (attributes.textAnchor == QLatin1String("middle"))
           fontStyle->setTextAnchor(Qt::AlignHCenter);
        else if (attributes.textAnchor == QLatin1String("end"))
           fontStyle->setTextAnchor(Qt::AlignRight);
    }

    node->appendStyleProperty(fontStyle, attributes.id);
}

static void parseTransform(QSvgNode *node,
                           const QSvgAttributes &attributes,
                           QSvgHandler *)
{
    if (attributes.transform.isEmpty())
        return;
    QTransform matrix = parseTransformationMatrix(attributes.transform.trimmed());

    if (!matrix.isIdentity()) {
        node->appendStyleProperty(new QSvgTransformStyle(QTransform(matrix)), attributes.id);
    }

}

static void parseVisibility(QSvgNode *node,
                            const QSvgAttributes &attributes,
                            QSvgHandler *)
{
    QSvgNode *parent = node->parent();

    if (parent && (attributes.visibility.isEmpty() || attributes.visibility == QT_INHERIT))
        node->setVisible(parent->isVisible());
    else if (attributes.visibility == QLatin1String("hidden") || attributes.visibility == QLatin1String("collapse")) {
        node->setVisible(false);
    } else
        node->setVisible(true);
}

static void pathArcSegment(QPainterPath &path,
                           qreal xc, qreal yc,
                           qreal th0, qreal th1,
                           qreal rx, qreal ry, qreal xAxisRotation)
{
    qreal sinTh, cosTh;
    qreal a00, a01, a10, a11;
    qreal x1, y1, x2, y2, x3, y3;
    qreal t;
    qreal thHalf;

    sinTh = qSin(xAxisRotation * (Q_PI / 180.0));
    cosTh = qCos(xAxisRotation * (Q_PI / 180.0));

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

// the arc handling code underneath is from XSVG (BSD license)
/*
 * Copyright  2002 USC/Information Sciences Institute
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Information Sciences Institute not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.  Information Sciences Institute
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * INFORMATION SCIENCES INSTITUTE DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL INFORMATION SCIENCES
 * INSTITUTE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
static void pathArc(QPainterPath &path,
                    qreal               rx,
                    qreal               ry,
                    qreal               x_axis_rotation,
                    int         large_arc_flag,
                    int         sweep_flag,
                    qreal               x,
                    qreal               y,
                    qreal curx, qreal cury)
{
    const qreal Pr1 = rx * rx;
    const qreal Pr2 = ry * ry;

    if (!Pr1 || !Pr2)
        return;

    qreal sin_th, cos_th;
    qreal a00, a01, a10, a11;
    qreal x0, y0, x1, y1, xc, yc;
    qreal d, sfactor, sfactor_sq;
    qreal th0, th1, th_arc;
    int i, n_segs;
    qreal dx, dy, dx1, dy1, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(x_axis_rotation * (Q_PI / 180.0));
    cos_th = qCos(x_axis_rotation * (Q_PI / 180.0));

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    if (!d)
        return;
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = qSqrt(sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = qAtan2(y0 - yc, x0 - xc);
    th1 = qAtan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * Q_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * Q_PI;

    n_segs = qCeil(qAbs(th_arc / (Q_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}

static bool parsePathDataFast(QStringView dataStr, QPainterPath &path, bool limitLength)
{
    const int maxElementCount = 0x7fff; // Assume file corruption if more path elements than this
    qreal x0 = 0, y0 = 0;              // starting point
    qreal x = 0, y = 0;                // current point
    char lastMode = 0;
    QPointF ctrlPt;
    const QChar *str = dataStr.constData();
    const QChar *end = str + dataStr.size();

    bool ok = true;
    while (ok && str != end) {
        while (str->isSpace() && (str + 1) != end)
            ++str;
        QChar pathElem = *str;
        ++str;
        QChar endc = *end;
        *const_cast<QChar *>(end) = u'\0'; // parseNumbersArray requires 0-termination that QStringView cannot guarantee
        const char *pattern = nullptr;
        if (pathElem == QLatin1Char('a') || pathElem == QLatin1Char('A'))
            pattern = "rrrffrr";
        QVarLengthArray<qreal, 8> arg;
        parseNumbersArray(str, arg, pattern);
        *const_cast<QChar *>(end) = endc;
        if (pathElem == QLatin1Char('z') || pathElem == QLatin1Char('Z'))
            arg.append(0);//dummy
        const qreal *num = arg.constData();
        int count = arg.size();
        while (ok && count > 0) {
            qreal offsetX = x;        // correction offsets
            qreal offsetY = y;        // for relative commands
            switch (pathElem.unicode()) {
            case 'm': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                x = x0 = num[0] + offsetX;
                y = y0 = num[1] + offsetY;
                num += 2;
                count -= 2;
                path.moveTo(x0, y0);

                 // As per 1.2  spec 8.3.2 The "moveto" commands
                 // If a 'moveto' is followed by multiple pairs of coordinates without explicit commands,
                 // the subsequent pairs shall be treated as implicit 'lineto' commands.
                 pathElem = QLatin1Char('l');
            }
                break;
            case 'M': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                x = x0 = num[0];
                y = y0 = num[1];
                num += 2;
                count -= 2;
                path.moveTo(x0, y0);

                // As per 1.2  spec 8.3.2 The "moveto" commands
                // If a 'moveto' is followed by multiple pairs of coordinates without explicit commands,
                // the subsequent pairs shall be treated as implicit 'lineto' commands.
                pathElem = QLatin1Char('L');
            }
                break;
            case 'z':
            case 'Z': {
                x = x0;
                y = y0;
                count--; // skip dummy
                num++;
                path.closeSubpath();
            }
                break;
            case 'l': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                x = num[0] + offsetX;
                y = num[1] + offsetY;
                num += 2;
                count -= 2;
                path.lineTo(x, y);

            }
                break;
            case 'L': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                x = num[0];
                y = num[1];
                num += 2;
                count -= 2;
                path.lineTo(x, y);
            }
                break;
            case 'h': {
                x = num[0] + offsetX;
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'H': {
                x = num[0];
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'v': {
                y = num[0] + offsetY;
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'V': {
                y = num[0];
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'c': {
                if (count < 6) {
                    ok = false;
                    break;
                }
                QPointF c1(num[0] + offsetX, num[1] + offsetY);
                QPointF c2(num[2] + offsetX, num[3] + offsetY);
                QPointF e(num[4] + offsetX, num[5] + offsetY);
                num += 6;
                count -= 6;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'C': {
                if (count < 6) {
                    ok = false;
                    break;
                }
                QPointF c1(num[0], num[1]);
                QPointF c2(num[2], num[3]);
                QPointF e(num[4], num[5]);
                num += 6;
                count -= 6;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 's': {
                if (count < 4) {
                    ok = false;
                    break;
                }
                QPointF c1;
                if (lastMode == 'c' || lastMode == 'C' ||
                    lastMode == 's' || lastMode == 'S')
                    c1 = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c1 = QPointF(x, y);
                QPointF c2(num[0] + offsetX, num[1] + offsetY);
                QPointF e(num[2] + offsetX, num[3] + offsetY);
                num += 4;
                count -= 4;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'S': {
                if (count < 4) {
                    ok = false;
                    break;
                }
                QPointF c1;
                if (lastMode == 'c' || lastMode == 'C' ||
                    lastMode == 's' || lastMode == 'S')
                    c1 = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c1 = QPointF(x, y);
                QPointF c2(num[0], num[1]);
                QPointF e(num[2], num[3]);
                num += 4;
                count -= 4;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'q': {
                if (count < 4) {
                    ok = false;
                    break;
                }
                QPointF c(num[0] + offsetX, num[1] + offsetY);
                QPointF e(num[2] + offsetX, num[3] + offsetY);
                num += 4;
                count -= 4;
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'Q': {
                if (count < 4) {
                    ok = false;
                    break;
                }
                QPointF c(num[0], num[1]);
                QPointF e(num[2], num[3]);
                num += 4;
                count -= 4;
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 't': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                QPointF e(num[0] + offsetX, num[1] + offsetY);
                num += 2;
                count -= 2;
                QPointF c;
                if (lastMode == 'q' || lastMode == 'Q' ||
                    lastMode == 't' || lastMode == 'T')
                    c = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c = QPointF(x, y);
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'T': {
                if (count < 2) {
                    ok = false;
                    break;
                }
                QPointF e(num[0], num[1]);
                num += 2;
                count -= 2;
                QPointF c;
                if (lastMode == 'q' || lastMode == 'Q' ||
                    lastMode == 't' || lastMode == 'T')
                    c = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c = QPointF(x, y);
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'a': {
                if (count < 7) {
                    ok = false;
                    break;
                }
                qreal rx = (*num++);
                qreal ry = (*num++);
                qreal xAxisRotation = (*num++);
                qreal largeArcFlag  = (*num++);
                qreal sweepFlag = (*num++);
                qreal ex = (*num++) + offsetX;
                qreal ey = (*num++) + offsetY;
                count -= 7;
                qreal curx = x;
                qreal cury = y;
                pathArc(path, rx, ry, xAxisRotation, int(largeArcFlag),
                        int(sweepFlag), ex, ey, curx, cury);

                x = ex;
                y = ey;
            }
                break;
            case 'A': {
                if (count < 7) {
                    ok = false;
                    break;
                }
                qreal rx = (*num++);
                qreal ry = (*num++);
                qreal xAxisRotation = (*num++);
                qreal largeArcFlag  = (*num++);
                qreal sweepFlag = (*num++);
                qreal ex = (*num++);
                qreal ey = (*num++);
                count -= 7;
                qreal curx = x;
                qreal cury = y;
                pathArc(path, rx, ry, xAxisRotation, int(largeArcFlag),
                        int(sweepFlag), ex, ey, curx, cury);

                x = ex;
                y = ey;
            }
                break;
            default:
                ok = false;
                break;
            }
            lastMode = pathElem.toLatin1();
            if (limitLength && path.elementCount() > maxElementCount)
                ok = false;
        }
    }
    return ok;
}

static bool parseStyle(QSvgNode *node,
                       const QXmlStreamAttributes &attributes,
                       QSvgHandler *);

static bool parseStyle(QSvgNode *node,
                       const QSvgAttributes &attributes,
                       QSvgHandler *);

#ifndef QT_NO_CSSPARSER

static void parseCSStoXMLAttrs(const QList<QCss::Declaration> &declarations,
                               QXmlStreamAttributes &attributes)
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

void QSvgHandler::parseCSStoXMLAttrs(const QString &css, QList<QSvgCssAttribute> *attributes)
{
    // preprocess (for unicode escapes), tokenize and remove comments
    m_cssParser.init(css);
    QString key;

    attributes->reserve(10);

    while (m_cssParser.hasNext()) {
        m_cssParser.skipSpace();

        if (!m_cssParser.hasNext())
            break;
        m_cssParser.next();

        QString name;
        if (m_cssParser.hasEscapeSequences) {
            key = m_cssParser.lexem();
            name = key;
        } else {
            const QCss::Symbol &sym = m_cssParser.symbol();
            name = sym.text.mid(sym.start, sym.len);
        }

        m_cssParser.skipSpace();
        if (!m_cssParser.test(QCss::COLON))
            break;

        m_cssParser.skipSpace();
        if (!m_cssParser.hasNext())
            break;

        QSvgCssAttribute attribute;
        attribute.name = name;

        const int firstSymbol = m_cssParser.index;
        int symbolCount = 0;
        do {
            m_cssParser.next();
            ++symbolCount;
        } while (m_cssParser.hasNext() && !m_cssParser.test(QCss::SEMICOLON));

        bool canExtractValueByRef = !m_cssParser.hasEscapeSequences;
        if (canExtractValueByRef) {
            int len = m_cssParser.symbols.at(firstSymbol).len;
            for (int i = firstSymbol + 1; i < firstSymbol + symbolCount; ++i) {
                len += m_cssParser.symbols.at(i).len;

                if (m_cssParser.symbols.at(i - 1).start + m_cssParser.symbols.at(i - 1).len
                        != m_cssParser.symbols.at(i).start) {
                    canExtractValueByRef = false;
                    break;
                }
            }
            if (canExtractValueByRef) {
                const QCss::Symbol &sym = m_cssParser.symbols.at(firstSymbol);
                attribute.value = sym.text.mid(sym.start, len);
            }
        }
        if (!canExtractValueByRef) {
            QString value;
            for (int i = firstSymbol; i < m_cssParser.index - 1; ++i)
                value += m_cssParser.symbols.at(i).lexem();
            attribute.value = value;
        }

        attributes->append(attribute);

        m_cssParser.skipSpace();
    }
}

static void cssStyleLookup(QSvgNode *node,
                           QSvgHandler *handler,
                           QSvgStyleSelector *selector,
                           QXmlStreamAttributes &attributes)
{
    QCss::StyleSelector::NodePtr cssNode;
    cssNode.ptr = node;
    QList<QCss::Declaration> decls = selector->declarationsForNode(cssNode);

    parseCSStoXMLAttrs(decls, attributes);
    parseStyle(node, attributes, handler);
}

static void cssStyleLookup(QSvgNode *node,
                           QSvgHandler *handler,
                           QSvgStyleSelector *selector)
{
    QXmlStreamAttributes attributes;
    cssStyleLookup(node, handler, selector, attributes);
}

#endif // QT_NO_CSSPARSER

QtSvg::Options QSvgHandler::options() const
{
    return m_options;
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
    QString xmlClassStr;

    for (int i = 0; i < attributes.size(); ++i) {
        const QXmlStreamAttribute &attribute = attributes.at(i);
        QStringView name = attribute.qualifiedName();
        if (name.isEmpty())
            continue;
        QStringView value = attribute.value();
        switch (name.at(0).unicode()) {
        case 'c':
            if (name == QLatin1String("class"))
                xmlClassStr = value.toString();
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
    node->setXmlClass(xmlClassStr);

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
        QSvgOpacityStyle *opacity = new QSvgOpacityStyle(qBound(qreal(0.0), op, qreal(1.0)));
        node->appendStyleProperty(opacity, attributes.id);
    }
}

static QPainter::CompositionMode svgToQtCompositionMode(const QString &op)
{
#define NOOP qDebug()<<"Operation: "<<op<<" is not implemented"
    if (op == QLatin1String("clear")) {
        return QPainter::CompositionMode_Clear;
    } else if (op == QLatin1String("src")) {
        return QPainter::CompositionMode_Source;
    } else if (op == QLatin1String("dst")) {
        return QPainter::CompositionMode_Destination;
    } else if (op == QLatin1String("src-over")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (op == QLatin1String("dst-over")) {
        return QPainter::CompositionMode_DestinationOver;
    } else if (op == QLatin1String("src-in")) {
        return QPainter::CompositionMode_SourceIn;
    } else if (op == QLatin1String("dst-in")) {
        return QPainter::CompositionMode_DestinationIn;
    } else if (op == QLatin1String("src-out")) {
        return QPainter::CompositionMode_SourceOut;
    } else if (op == QLatin1String("dst-out")) {
        return QPainter::CompositionMode_DestinationOut;
    } else if (op == QLatin1String("src-atop")) {
        return QPainter::CompositionMode_SourceAtop;
    } else if (op == QLatin1String("dst-atop")) {
        return QPainter::CompositionMode_DestinationAtop;
    } else if (op == QLatin1String("xor")) {
        return QPainter::CompositionMode_Xor;
    } else if (op == QLatin1String("plus")) {
        return QPainter::CompositionMode_Plus;
    } else if (op == QLatin1String("multiply")) {
        return QPainter::CompositionMode_Multiply;
    } else if (op == QLatin1String("screen")) {
        return QPainter::CompositionMode_Screen;
    } else if (op == QLatin1String("overlay")) {
        return QPainter::CompositionMode_Overlay;
    } else if (op == QLatin1String("darken")) {
        return QPainter::CompositionMode_Darken;
    } else if (op == QLatin1String("lighten")) {
        return QPainter::CompositionMode_Lighten;
    } else if (op == QLatin1String("color-dodge")) {
        return QPainter::CompositionMode_ColorDodge;
    } else if (op == QLatin1String("color-burn")) {
        return QPainter::CompositionMode_ColorBurn;
    } else if (op == QLatin1String("hard-light")) {
        return QPainter::CompositionMode_HardLight;
    } else if (op == QLatin1String("soft-light")) {
        return QPainter::CompositionMode_SoftLight;
    } else if (op == QLatin1String("difference")) {
        return QPainter::CompositionMode_Difference;
    } else if (op == QLatin1String("exclusion")) {
        return QPainter::CompositionMode_Exclusion;
    } else {
        NOOP;
    }

    return QPainter::CompositionMode_SourceOver;
}

static void parseCompOp(QSvgNode *node,
                        const QSvgAttributes &attributes,
                        QSvgHandler *)
{
    if (attributes.compOp.isEmpty())
        return;
    QString value = attributes.compOp.toString().trimmed();

    if (!value.isEmpty()) {
        QSvgCompOpStyle *compop = new QSvgCompOpStyle(svgToQtCompositionMode(value));
        node->appendStyleProperty(compop, attributes.id);
    }
}

static inline QSvgNode::DisplayMode displayStringToEnum(const QString &str)
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
    } else if (str == QT_INHERIT) {
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
    QString displayStr = attributes.display.toString().trimmed();

    if (!displayStr.isEmpty()) {
        node->setDisplayMode(displayStringToEnum(displayStr));
    }
}

static void parseExtendedAttributes(QSvgNode *node,
                                    const QSvgAttributes &attributes,
                                    QSvgHandler *handler)
{
    if (handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
        return;

    if (!attributes.mask.isEmpty()) {
        QString maskStr = attributes.mask.toString().trimmed();
        if (maskStr.size() > 3 && maskStr.mid(0, 3) == QLatin1String("url"))
            maskStr = maskStr.mid(3, maskStr.size() - 3);
        QString maskId = idFromUrl(maskStr);
        if (maskId.startsWith(QLatin1Char('#'))) //TODO: handle urls and ids in a single place
            maskId.remove(0, 1);

        node->setMaskId(maskId);
    }

    if (!attributes.markerStart.isEmpty() &&
        !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly)) {
        QString markerStr = attributes.markerStart.toString().trimmed();
        if (markerStr.size() > 3 && markerStr.mid(0, 3) == QLatin1String("url"))
            markerStr = markerStr.mid(3, markerStr.size() - 3);
        QString markerId = idFromUrl(markerStr);
        if (markerId.startsWith(QLatin1Char('#'))) //TODO: handle urls and ids in a single place
            markerId.remove(0, 1);
        node->setMarkerStartId(markerId);
    }
    if (!attributes.markerMid.isEmpty() &&
        !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly)) {
        QString markerStr = attributes.markerMid.toString().trimmed();
        if (markerStr.size() > 3 && markerStr.mid(0, 3) == QLatin1String("url"))
            markerStr = markerStr.mid(3, markerStr.size() - 3);
        QString markerId = idFromUrl(markerStr);
        if (markerId.startsWith(QLatin1Char('#'))) //TODO: handle urls and ids in a single place
            markerId.remove(0, 1);
        node->setMarkerMidId(markerId);
    }
    if (!attributes.markerEnd.isEmpty() &&
        !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly)) {
        QString markerStr = attributes.markerEnd.toString().trimmed();
        if (markerStr.size() > 3 && markerStr.mid(0, 3) == QLatin1String("url"))
            markerStr = markerStr.mid(3, markerStr.size() - 3);
        QString markerId = idFromUrl(markerStr);
        if (markerId.startsWith(QLatin1Char('#'))) //TODO: handle urls and ids in a single place
            markerId.remove(0, 1);
        node->setMarkerEndId(markerId);
    }

    if (!attributes.filter.isEmpty() &&
        !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly)) {
        QString filterStr = attributes.filter.toString().trimmed();

        if (filterStr.size() > 3 && filterStr.mid(0, 3) == QLatin1String("url"))
            filterStr = filterStr.mid(3, filterStr.size() - 3);
        QString filterId = idFromUrl(filterStr);
        if (filterId.startsWith(QLatin1Char('#'))) //TODO: handle urls and ids in a single place
            filterId.remove(0, 1);
        node->setFilterId(filterId);
    }

}

static void parseRenderingHints(QSvgNode *node,
                                const QSvgAttributes &attributes,
                                QSvgHandler *)
{
    if (attributes.imageRendering.isEmpty())
        return;

    QString ir = attributes.imageRendering.toString().trimmed();
    QSvgQualityStyle *p = new QSvgQualityStyle(0);
    if (ir == QLatin1String("auto"))
        p->setImageRendering(QSvgQualityStyle::ImageRenderingAuto);
    else if (ir == QLatin1String("optimizeSpeed"))
        p->setImageRendering(QSvgQualityStyle::ImageRenderingOptimizeSpeed);
    else if (ir == QLatin1String("optimizeQuality"))
        p->setImageRendering(QSvgQualityStyle::ImageRenderingOptimizeQuality);
    node->appendStyleProperty(p, attributes.id);
}


static bool parseStyle(QSvgNode *node,
                       const QSvgAttributes &attributes,
                       QSvgHandler *handler)
{
    parseColor(node, attributes, handler);
    parseBrush(node, attributes, handler);
    parsePen(node, attributes, handler);
    parseFont(node, attributes, handler);
    parseTransform(node, attributes, handler);
    parseVisibility(node, attributes, handler);
    parseOpacity(node, attributes, handler);
    parseCompOp(node, attributes, handler);
    parseRenderingHints(node, attributes, handler);
    parseOthers(node, attributes, handler);
    parseExtendedAttributes(node, attributes, handler);

#if 0
    value = attributes.value("audio-level");

    value = attributes.value("color-rendering");

    value = attributes.value("display-align");

    value = attributes.value("image-rendering");

    value = attributes.value("line-increment");

    value = attributes.value("pointer-events");

    value = attributes.value("shape-rendering");

    value = attributes.value("solid-color");

    value = attributes.value("solid-opacity");

    value = attributes.value("text-rendering");

    value = attributes.value("vector-effect");

    value = attributes.value("viewport-fill");

    value = attributes.value("viewport-fill-opacity");
#endif
    return true;
}

static bool parseStyle(QSvgNode *node,
                       const QXmlStreamAttributes &attrs,
                       QSvgHandler *handler)
{
    return parseStyle(node, QSvgAttributes(attrs, handler), handler);
}

static bool parseAnchorNode(QSvgNode *parent,
                            const QXmlStreamAttributes &attributes,
                            QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseAnimateNode(QSvgNode *parent,
                             const QXmlStreamAttributes &attributes,
                             QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

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
    double val = ms * toDouble(str, ok);
    if (ok) {
        if (val > std::numeric_limits<int>::min() && val < std::numeric_limits<int>::max())
            res = static_cast<int>(val);
        else
            *ok = false;
    }
    return res;
}

static bool parseAnimateColorNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *handler)
{
    QStringView fromStr    = attributes.value(QLatin1String("from"));
    QStringView toStr      = attributes.value(QLatin1String("to"));
    QString valuesStr  = attributes.value(QLatin1String("values")).toString();
    QString beginStr   = attributes.value(QLatin1String("begin")).toString();
    QString durStr     = attributes.value(QLatin1String("dur")).toString();
    QString targetStr  = attributes.value(QLatin1String("attributeName")).toString();
    QString repeatStr  = attributes.value(QLatin1String("repeatCount")).toString();
    QString fillStr    = attributes.value(QLatin1String("fill")).toString();

    if (targetStr != QLatin1String("fill") && targetStr != QLatin1String("stroke"))
        return false;

    QList<QColor> colors;
    if (valuesStr.isEmpty()) {
        QColor startColor, endColor;
        resolveColor(fromStr, startColor, handler);
        resolveColor(toStr, endColor, handler);
        colors.reserve(2);
        colors.append(startColor);
        colors.append(endColor);
    } else {
        QStringList str = valuesStr.split(QLatin1Char(';'));
        colors.reserve(str.size());
        QStringList::const_iterator itr;
        for (itr = str.constBegin(); itr != str.constEnd(); ++itr) {
            QColor color;
            resolveColor(*itr, color, handler);
            colors.append(color);
        }
    }

    bool ok = true;
    int begin = parseClockValue(beginStr, &ok);
    if (!ok)
        return false;
    int end = begin + parseClockValue(durStr, &ok);
    if (!ok || end <= begin)
        return false;

    QSvgAnimateColor *anim = new QSvgAnimateColor(begin, end, 0);
    anim->setArgs((targetStr == QLatin1String("fill")), colors);
    anim->setFreeze(fillStr == QLatin1String("freeze"));
    anim->setRepeatCount(
        (repeatStr == QLatin1String("indefinite")) ? -1 :
            (repeatStr == QLatin1String("")) ? 1 : toDouble(repeatStr));

    parent->appendStyleProperty(anim, someId(attributes));
    parent->document()->setAnimated(true);
    handler->setAnimPeriod(begin, end);
    return true;
}

static bool parseAimateMotionNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static void parseNumberTriplet(QList<qreal> &values, const QChar *&s)
{
    QList<qreal> list = parseNumbersList(s);
    values << list;
    for (int i = 3 - list.size(); i > 0; --i)
        values.append(0.0);
}

static bool parseAnimateTransformNode(QSvgNode *parent,
                                      const QXmlStreamAttributes &attributes,
                                      QSvgHandler *handler)
{
    QString typeStr    = attributes.value(QLatin1String("type")).toString();
    QString values     = attributes.value(QLatin1String("values")).toString();
    QString beginStr   = attributes.value(QLatin1String("begin")).toString();
    QString durStr     = attributes.value(QLatin1String("dur")).toString();
    QString repeatStr  = attributes.value(QLatin1String("repeatCount")).toString();
    QString fillStr    = attributes.value(QLatin1String("fill")).toString();
    QString fromStr    = attributes.value(QLatin1String("from")).toString();
    QString toStr      = attributes.value(QLatin1String("to")).toString();
    QString byStr      = attributes.value(QLatin1String("by")).toString();
    QString addtv      = attributes.value(QLatin1String("additive")).toString();

    QSvgAnimateTransform::Additive additive = QSvgAnimateTransform::Replace;
    if (addtv == QLatin1String("sum"))
        additive = QSvgAnimateTransform::Sum;

    QList<qreal> vals;
    if (values.isEmpty()) {
        const QChar *s;
        if (fromStr.isEmpty()) {
            if (!byStr.isEmpty()) {
                // By-animation.
                additive = QSvgAnimateTransform::Sum;
                vals.append(0.0);
                vals.append(0.0);
                vals.append(0.0);
                parseNumberTriplet(vals, s = byStr.constData());
            } else {
                // To-animation not defined.
                return false;
            }
        } else {
            if (!toStr.isEmpty()) {
                // From-to-animation.
                parseNumberTriplet(vals, s = fromStr.constData());
                parseNumberTriplet(vals, s = toStr.constData());
            } else if (!byStr.isEmpty()) {
                // From-by-animation.
                parseNumberTriplet(vals, s = fromStr.constData());
                parseNumberTriplet(vals, s = byStr.constData());
                for (int i = vals.size() - 3; i < vals.size(); ++i)
                    vals[i] += vals[i - 3];
            } else {
                return false;
            }
        }
    } else {
        const QChar *s = values.constData();
        while (s && *s != QLatin1Char(0)) {
            parseNumberTriplet(vals, s);
            if (*s == QLatin1Char(0))
                break;
            ++s;
        }
    }
    if (vals.size() % 3 != 0)
        return false;

    bool ok = true;
    int begin = parseClockValue(beginStr, &ok);
    if (!ok)
        return false;
    int end = begin + parseClockValue(durStr, &ok);
    if (!ok || end <= begin)
        return false;

    QSvgAnimateTransform::TransformType type = QSvgAnimateTransform::Empty;
    if (typeStr == QLatin1String("translate")) {
        type = QSvgAnimateTransform::Translate;
    } else if (typeStr == QLatin1String("scale")) {
        type = QSvgAnimateTransform::Scale;
    } else if (typeStr == QLatin1String("rotate")) {
        type = QSvgAnimateTransform::Rotate;
    } else if (typeStr == QLatin1String("skewX")) {
        type = QSvgAnimateTransform::SkewX;
    } else if (typeStr == QLatin1String("skewY")) {
        type = QSvgAnimateTransform::SkewY;
    } else {
        return false;
    }

    QSvgAnimateTransform *anim = new QSvgAnimateTransform(begin, end, 0);
    anim->setArgs(type, additive, vals);
    anim->setFreeze(fillStr == QLatin1String("freeze"));
    anim->setRepeatCount(
            (repeatStr == QLatin1String("indefinite"))? -1 :
            (repeatStr == QLatin1String(""))? 1 : toDouble(repeatStr));

    parent->appendStyleProperty(anim, someId(attributes));
    parent->document()->setAnimated(true);
    handler->setAnimPeriod(begin, end);
    return true;
}

static QSvgNode * createAnimationNode(QSvgNode *parent,
                                      const QXmlStreamAttributes &attributes,
                                      QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return 0;
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
    qreal ncx = toDouble(cx);
    qreal ncy = toDouble(cy);
    qreal nr  = toDouble(r);
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
    qreal ncx = toDouble(cx);
    qreal ncy = toDouble(cy);
    qreal nrx = toDouble(rx);
    qreal nry = toDouble(ry);

    QRectF rect(ncx-nrx, ncy-nry, nrx*2, nry*2);
    QSvgNode *ellipse = new QSvgEllipse(parent, rect);
    return ellipse;
}

static QSvgStyleProperty *createFontNode(QSvgNode *parent,
                                         const QXmlStreamAttributes &attributes,
                                         QSvgHandler *)
{
    const QStringView hax = attributes.value(QLatin1String("horiz-adv-x"));
    QString myId     = someId(attributes);

    qreal horizAdvX = toDouble(hax);

    while (parent && parent->type() != QSvgNode::Doc) {
        parent = parent->parent();
    }

    if (parent && !myId.isEmpty()) {
        QSvgTinyDocument *doc = static_cast<QSvgTinyDocument*>(parent);
        QSvgFont *font = doc->svgFont(myId);
        if (!font) {
            font = new QSvgFont(horizAdvX);
            font->setFamilyName(myId);
            doc->addSvgFont(font);
        }
        return new QSvgFontStyle(font, doc);
    }
    return nullptr;
}

static bool parseFontFaceNode(QSvgStyleProperty *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *)
{
    if (parent->type() != QSvgStyleProperty::FONT) {
        return false;
    }

    QSvgFontStyle *style = static_cast<QSvgFontStyle*>(parent);
    QSvgFont *font = style->svgFont();
    QString name   = attributes.value(QLatin1String("font-family")).toString();
    const QStringView unitsPerEmStr = attributes.value(QLatin1String("units-per-em"));

    qreal unitsPerEm = toDouble(unitsPerEmStr);
    if (!unitsPerEm)
        unitsPerEm = QSvgFont::DEFAULT_UNITS_PER_EM;

    if (!name.isEmpty())
        font->setFamilyName(name);
    font->setUnitsPerEm(unitsPerEm);

    if (!font->familyName().isEmpty())
        if (!style->doc()->svgFont(font->familyName()))
            style->doc()->addSvgFont(font);

    return true;
}

static bool parseFontFaceNameNode(QSvgStyleProperty *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    if (parent->type() != QSvgStyleProperty::FONT) {
        return false;
    }

    QSvgFontStyle *style = static_cast<QSvgFontStyle*>(parent);
    QSvgFont *font = style->svgFont();
    QString name   = attributes.value(QLatin1String("name")).toString();

    if (!name.isEmpty())
        font->setFamilyName(name);

    if (!font->familyName().isEmpty())
        if (!style->doc()->svgFont(font->familyName()))
            style->doc()->addSvgFont(font);

    return true;
}

static bool parseFontFaceSrcNode(QSvgStyleProperty *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseFontFaceUriNode(QSvgStyleProperty *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
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

static bool parseGlyphNode(QSvgStyleProperty *parent,
                           const QXmlStreamAttributes &attributes,
                           QSvgHandler *)
{
    if (parent->type() != QSvgStyleProperty::FONT) {
        return false;
    }

    QSvgFontStyle *style = static_cast<QSvgFontStyle*>(parent);
    QSvgFont *font = style->svgFont();
    createSvgGlyph(font, attributes);
    return true;
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
    const QStringView x = attributes.value(QLatin1String("x"));
    const QStringView y = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    QString filename = attributes.value(QLatin1String("xlink:href")).toString();
    if (filename.isEmpty() && !handler->options().testFlag(QtSvg::Tiny12FeaturesOnly))
        filename = attributes.value(QLatin1String("href")).toString();
    qreal nx = toDouble(x);
    qreal ny = toDouble(y);
    QSvgHandler::LengthType type;
    qreal nwidth = parseLength(width.toString(), &type, handler);
    nwidth = convertToPixels(nwidth, true, type);

    qreal nheight = parseLength(height.toString(), &type, handler);
    nheight = convertToPixels(nheight, false, type);

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
        int idx = filename.lastIndexOf(QLatin1String("base64,"));
        if (idx != -1) {
            idx += 7;
            const QString dataStr = filename.mid(idx);
            QByteArray data = QByteArray::fromBase64(dataStr.toLatin1());
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

    QSvgNode *img = new QSvgImage(parent,
                                  image,
                                  filenameType == LoadedFromFile ? filename : QString{},
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
    qreal nx1 = toDouble(x1);
    qreal ny1 = toDouble(y1);
    qreal nx2 = toDouble(x2);
    qreal ny2 = toDouble(y2);

    QLineF lineBounds(nx1, ny1, nx2, ny2);
    QSvgNode *line = new QSvgLine(parent, lineBounds);
    return line;
}


static void parseBaseGradient(QSvgNode *node,
                              const QXmlStreamAttributes &attributes,
                              QSvgGradientStyle *gradProp,
                              QSvgHandler *handler)
{
    QString link   = attributes.value(QLatin1String("xlink:href")).toString();
    QStringView trans  = attributes.value(QLatin1String("gradientTransform"));
    QString spread = attributes.value(QLatin1String("spreadMethod")).toString();
    QString units = attributes.value(QLatin1String("gradientUnits")).toString();
    QStringView colorStr = attributes.value(QLatin1String("color"));
    QStringView colorOpacityStr = attributes.value(QLatin1String("color-opacity"));

    QColor color;
    if (constructColor(colorStr, colorOpacityStr, color, handler)) {
        handler->popColor();
        handler->pushColor(color);
    }

    QTransform matrix;
    QGradient *grad = gradProp->qgradient();
    if (node && !link.isEmpty()) {
        QSvgStyleProperty *prop = node->styleProperty(link);
        //qDebug()<<"inherited "<<prop<<" ("<<link<<")";
        if (prop && prop->type() == QSvgStyleProperty::GRADIENT) {
            QSvgGradientStyle *inherited =
                static_cast<QSvgGradientStyle*>(prop);
            if (!inherited->stopLink().isEmpty()) {
                gradProp->setStopLink(inherited->stopLink(), handler->document());
            } else {
                grad->setStops(inherited->qgradient()->stops());
                gradProp->setGradientStopsSet(inherited->gradientStopsSet());
            }

            matrix = inherited->qtransform();
        } else {
            gradProp->setStopLink(link, handler->document());
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

static QSvgStyleProperty *createLinearGradientNode(QSvgNode *node,
                                                   const QXmlStreamAttributes &attributes,
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
        nx1 =  convertToNumber(x1, handler);
    if (!y1.isEmpty())
        ny1 =  convertToNumber(y1, handler);
    if (!x2.isEmpty())
        nx2 =  convertToNumber(x2, handler);
    if (!y2.isEmpty())
        ny2 =  convertToNumber(y2, handler);

    QSvgNode *itr = node;
    while (itr && itr->type() != QSvgNode::Doc) {
        itr = itr->parent();
    }

    QLinearGradient *grad = new QLinearGradient(nx1, ny1, nx2, ny2);
    grad->setInterpolationMode(QGradient::ComponentInterpolation);
    QSvgGradientStyle *prop = new QSvgGradientStyle(grad);
    parseBaseGradient(node, attributes, prop, handler);

    return prop;
}

static bool parseMetadataNode(QSvgNode *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static bool parseMissingGlyphNode(QSvgStyleProperty *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *)
{
    if (parent->type() != QSvgStyleProperty::FONT) {
        return false;
    }

    QSvgFontStyle *style = static_cast<QSvgFontStyle*>(parent);
    QSvgFont *font = style->svgFont();
    createSvgGlyph(font, attributes);
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
    QSvgHandler::LengthType type;

    QtSvg::UnitTypes nmUx = nmU;
    QtSvg::UnitTypes nmUy = nmU;
    QtSvg::UnitTypes nmUw = nmU;
    QtSvg::UnitTypes nmUh = nmU;
    qreal nx = parseLength(x.toString(), &type, handler, &ok);
    nx = convertToPixels(nx, true, type);
    if (x.isEmpty() || !ok) {
        nx = -0.1;
        nmUx = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QSvgHandler::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nx = nx / 100. * parent->document()->viewBox().width();
    } else if (type == QSvgHandler::LT_PERCENT) {
        nx = nx / 100.;
    }

    qreal ny = parseLength(y.toString(), &type, handler, &ok);
    ny = convertToPixels(ny, true, type);
    if (y.isEmpty() || !ok) {
        ny = -0.1;
        nmUy = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QSvgHandler::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        ny = ny / 100. * parent->document()->viewBox().height();
    } else if (type == QSvgHandler::LT_PERCENT) {
        ny = ny / 100.;
    }

    qreal nwidth = parseLength(width.toString(), &type, handler, &ok);
    nwidth = convertToPixels(nwidth, true, type);
    if (width.isEmpty() || !ok) {
        nwidth = 1.2;
        nmUw = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QSvgHandler::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nwidth = nwidth / 100. * parent->document()->viewBox().width();
    } else if (type == QSvgHandler::LT_PERCENT) {
        nwidth = nwidth / 100.;
    }

    qreal nheight = parseLength(height.toString(), &type, handler, &ok);
    nheight = convertToPixels(nheight, true, type);
    if (height.isEmpty() || !ok) {
        nheight = 1.2;
        nmUh = QtSvg::UnitTypes::objectBoundingBox;
    } else if (type == QSvgHandler::LT_PERCENT && nmU == QtSvg::UnitTypes::userSpaceOnUse) {
        nheight = nheight / 100. * parent->document()->viewBox().height();
    } else if (type == QSvgHandler::LT_PERCENT) {
        nheight = nheight / 100.;
    }

    QRectF bounds(nx, ny, nwidth, nheight);
    if (bounds.isEmpty())
        return nullptr;

    QSvgNode *mask = new QSvgMask(parent, QSvgRectF(bounds, nmUx, nmUy, nmUw, nmUh), nmCU);

    return mask;
}

static void parseFilterBounds(QSvgNode *, const QXmlStreamAttributes &attributes,
                              QSvgHandler *handler, QSvgRectF *rect)
{
    const QStringView xStr        = attributes.value(QLatin1String("x"));
    const QStringView yStr        = attributes.value(QLatin1String("y"));
    const QStringView widthStr    = attributes.value(QLatin1String("width"));
    const QStringView heightStr   = attributes.value(QLatin1String("height"));

    qreal x = 0;
    if (!xStr.isEmpty()) {
        QSvgHandler::LengthType type;
        x = parseLength(xStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT) {
            x = convertToPixels(x, true, type);
            rect->setUnitX(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QSvgHandler::LT_PERCENT) {
            x /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setX(x);
    }
    qreal y = 0;
    if (!yStr.isEmpty()) {
        QSvgHandler::LengthType type;
        y = parseLength(yStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT) {
            y = convertToPixels(y, false, type);
            rect->setUnitY(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QSvgHandler::LT_PERCENT) {
            y /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setY(y);
    }
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        QSvgHandler::LengthType type;
        width = parseLength(widthStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT) {
            width = convertToPixels(width, true, type);
            rect->setUnitW(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QSvgHandler::LT_PERCENT) {
            width /= 100.;
            rect->setUnitX(QtSvg::UnitTypes::objectBoundingBox);
        }
        rect->setWidth(width);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        QSvgHandler::LengthType type;
        height = parseLength(heightStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT) {
            height = convertToPixels(height, false, type);
            rect->setUnitH(QtSvg::UnitTypes::userSpaceOnUse);
        }
        if (type == QSvgHandler::LT_PERCENT) {
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
    QString fU = attributes.value(QLatin1String("filterUnits")).toString();
    QString pU = attributes.value(QLatin1String("primitiveUnits")).toString();

    QtSvg::UnitTypes filterUnits = fU.contains(QLatin1String("userSpaceOnUse")) ?
                QtSvg::UnitTypes::userSpaceOnUse : QtSvg::UnitTypes::objectBoundingBox;

    QtSvg::UnitTypes primitiveUnits = pU.contains(QLatin1String("objectBoundingBox")) ?
                QtSvg::UnitTypes::objectBoundingBox : QtSvg::UnitTypes::userSpaceOnUse;

    // https://www.w3.org/TR/SVG11/filters.html#FilterEffectsRegion
    // If ‘x’ or ‘y’ is not specified, the effect is as if a value of -10% were specified.
    // If ‘width’ or ‘height’ is not specified, the effect is as if a value of 120% were specified.
    QSvgRectF rect;
    if (filterUnits == QtSvg::UnitTypes::userSpaceOnUse) {
        qreal width = parent->document()->viewBox().width();
        qreal height = parent->document()->viewBox().height();
        rect = QSvgRectF(QRectF(-0.1 * width, -0.1 * height, 1.2 * width, 1.2 * height),
                         QtSvg::UnitTypes::userSpaceOnUse, QtSvg::UnitTypes::userSpaceOnUse,
                         QtSvg::UnitTypes::userSpaceOnUse, QtSvg::UnitTypes::userSpaceOnUse);
    } else {
        rect = QSvgRectF(QRectF(-0.1, -0.1, 1.2, 1.2),
                         QtSvg::UnitTypes::objectBoundingBox, QtSvg::UnitTypes::objectBoundingBox,
                         QtSvg::UnitTypes::objectBoundingBox, QtSvg::UnitTypes::objectBoundingBox);
    }

    parseFilterBounds(parent, attributes, handler, &rect);

    QSvgNode *filter = new QSvgFilterContainer(parent, rect, filterUnits, primitiveUnits);
    return filter;
}

static void parseFilterAttributes(QSvgNode *parent, const QXmlStreamAttributes &attributes,
                                  QSvgHandler *handler, QString *inString, QString *outString,
                                  QSvgRectF *rect)
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

    parseFilterBounds(parent, attributes, handler, rect);
}

static QSvgNode *createFeColorMatrixNode(QSvgNode *parent,
                                        const QXmlStreamAttributes &attributes,
                                        QSvgHandler *handler)
{
    const QString typeString = attributes.value(QLatin1String("type")).toString();
    QString valuesString = attributes.value(QLatin1String("values")).toString();

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    QSvgFeColorMatrix::ColorShiftType type;
    QSvgFeColorMatrix::Matrix values;
    values.fill(0);

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    if (typeString.startsWith(QLatin1String("saturate")))
        type = QSvgFeColorMatrix::ColorShiftType::Saturate;
    else if (typeString.startsWith(QLatin1String("hueRotate")))
        type = QSvgFeColorMatrix::ColorShiftType::HueRotate;
    else if (typeString.startsWith(QLatin1String("luminanceToAlpha")))
        type = QSvgFeColorMatrix::ColorShiftType::LuminanceToAlpha;
    else
        type = QSvgFeColorMatrix::ColorShiftType::Matrix;

    if (!valuesString.isEmpty()) {
        static QRegularExpression delimiterRE(QLatin1String("[,\\s]"));
        const QStringList valueStringList = valuesString.split(delimiterRE, Qt::SkipEmptyParts);

        for (int i = 0, j = 0; i < qMin(20, valueStringList.size()); i++) {
            bool ok;
            qreal v = toDouble(valueStringList.at(i), &ok);
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
                                          QSvgHandler *handler)
{
    const QString edgeModeString    = attributes.value(QLatin1String("edgeMode")).toString();
    QString stdDeviationString  = attributes.value(QLatin1String("stdDeviation")).toString();

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    QSvgFeGaussianBlur::EdgeMode edgemode = QSvgFeGaussianBlur::EdgeMode::Duplicate;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);
    qreal stdDeviationX = 0;
    qreal stdDeviationY = 0;
    if (stdDeviationString.contains(QStringLiteral(" "))){
        stdDeviationX = qMax(0., toDouble(stdDeviationString.split(QStringLiteral(" ")).first()));
        stdDeviationY = qMax(0., toDouble(stdDeviationString.split(QStringLiteral(" ")).last()));
    } else {
        stdDeviationY = stdDeviationX = qMax(0., toDouble(stdDeviationString));
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
                                    QSvgHandler *handler)
{
    QStringView dxString = attributes.value(QLatin1String("dx"));
    QStringView dyString = attributes.value(QLatin1String("dy"));

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    qreal dx = 0;
    if (!dxString.isEmpty()) {
        QSvgHandler::LengthType type;
        dx = parseLength(dxString.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            dx = convertToPixels(dx, true, type);
    }

    qreal dy = 0;
    if (!dyString.isEmpty()) {
        QSvgHandler::LengthType type;
        dy = parseLength(dyString.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            dy = convertToPixels(dy, true, type);
    }

    QSvgNode *filter = new QSvgFeOffset(parent, inputString, outputString, rect,
                                        dx, dy);
    return filter;
}

static QSvgNode *createFeCompositeNode(QSvgNode *parent,
                                  const QXmlStreamAttributes &attributes,
                                  QSvgHandler *handler)
{
    QString in2String        = attributes.value(QLatin1String("in2")).toString();
    QString operatorString   = attributes.value(QLatin1String("operator")).toString();
    QString k1String         = attributes.value(QLatin1String("k1")).toString();
    QString k2String         = attributes.value(QLatin1String("k2")).toString();
    QString k3String         = attributes.value(QLatin1String("k3")).toString();
    QString k4String         = attributes.value(QLatin1String("k4")).toString();

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    QSvgFeComposite::Operator op = QSvgFeComposite::Operator::Over;
    if (operatorString.startsWith(QStringLiteral("in")))
        op = QSvgFeComposite::Operator::In;
    else if (operatorString.startsWith(QStringLiteral("out")))
        op = QSvgFeComposite::Operator::Out;
    else if (operatorString.startsWith(QStringLiteral("atop")))
        op = QSvgFeComposite::Operator::Atop;
    else if (operatorString.startsWith(QStringLiteral("xor")))
        op = QSvgFeComposite::Operator::Xor;
    else if (operatorString.startsWith(QStringLiteral("lighter")))
        op = QSvgFeComposite::Operator::Lighter;
    else if (operatorString.startsWith(QStringLiteral("arithmetic")))
        op = QSvgFeComposite::Operator::Arithmetic;

    QVector4D k(0, 0, 0, 0);

    if (op == QSvgFeComposite::Operator::Arithmetic) {
        bool ok;
        qreal v = toDouble(k1String, &ok);
        if (ok)
            k.setX(v);
        v = toDouble(k2String, &ok);
        if (ok)
            k.setY(v);
        v = toDouble(k3String, &ok);
        if (ok)
            k.setZ(v);
        v = toDouble(k4String, &ok);
        if (ok)
            k.setW(v);
    }

    QSvgNode *filter = new QSvgFeComposite(parent, inputString, outputString, rect,
                                           in2String, op, k);
    return filter;
}


static QSvgNode *createFeMergeNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *handler)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

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
        bool ok;
        qreal op = qMin(qreal(1.0), qMax(qreal(0.0), toDouble(opacityStr, &ok)));
        if (ok)
            color.setAlphaF(op);
    }

    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeFlood(parent, inputString, outputString, rect, color);
    return filter;
}

static QSvgNode *createFeMergeNodeNode(QSvgNode *parent,
                                       const QXmlStreamAttributes &attributes,
                                       QSvgHandler *handler)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeMergeNode(parent, inputString, outputString, rect);
    return filter;
}

static QSvgNode *createFeUnsupportedNode(QSvgNode *parent,
                                         const QXmlStreamAttributes &attributes,
                                         QSvgHandler *handler)
{
    QString inputString;
    QString outputString;
    QSvgRectF rect;

    parseFilterAttributes(parent, attributes, handler,
                          &inputString, &outputString, &rect);

    QSvgNode *filter = new QSvgFeUnsupported(parent, inputString, outputString, rect);
    return filter;
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
    const QStringView widthStr    = attributes.value(QLatin1String(marker ? "markerWidth":"width"));
    const QStringView heightStr   = attributes.value(QLatin1String(marker ? "markerHeight":"height"));
    const QString pAspectRStr     = attributes.value(QLatin1String("preserveAspectRatio")).toString();
    const QStringView overflowStr = attributes.value(QLatin1String("overflow"));

    QString viewBoxStr = attributes.value(QLatin1String("viewBox")).toString();


    qreal x = 0;
    if (!xStr.isEmpty()) {
        QSvgHandler::LengthType type;
        x = parseLength(xStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            x = convertToPixels(x, true, type);
    }
    qreal y = 0;
    if (!yStr.isEmpty()) {
        QSvgHandler::LengthType type;
        y = parseLength(yStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            y = convertToPixels(y, false, type);
    }
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        QSvgHandler::LengthType type;
        width = parseLength(widthStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            width = convertToPixels(width, true, type);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        QSvgHandler::LengthType type;
        height = parseLength(heightStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            height = convertToPixels(height, false, type);
    }

    *rect = QRectF(x, y, width, height);

    x = 0;
    if (!refXStr.isEmpty()) {
        QSvgHandler::LengthType type;
        x = parseLength(refXStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            x = convertToPixels(x, true, type);
    }
    y = 0;
    if (!refYStr.isEmpty()) {
        QSvgHandler::LengthType type;
        y = parseLength(refYStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            y = convertToPixels(y, false, type);
    }
    *refPoint = QPointF(x,y);

    QStringList viewBoxValues;
    if (!viewBoxStr.isEmpty()) {
        viewBoxStr = viewBoxStr.replace(QLatin1Char(' '), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\r'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\n'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\t'), QLatin1Char(','));
        viewBoxValues = viewBoxStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (viewBoxValues.size() == 4) {
        QString xStr      = viewBoxValues.at(0).trimmed();
        QString yStr      = viewBoxValues.at(1).trimmed();
        QString widthStr  = viewBoxValues.at(2).trimmed();
        QString heightStr = viewBoxValues.at(3).trimmed();

        QSvgHandler::LengthType lt;
        qreal x = parseLength(xStr, &lt, handler);
        qreal y = parseLength(yStr, &lt, handler);
        qreal w = parseLength(widthStr, &lt, handler);
        qreal h = parseLength(heightStr, &lt, handler);

        *viewBox = QRectF(x, y, w, h);

    } else if (width > 0 && height > 0) {
        *viewBox = QRectF(0, 0, width, height);
    } else {
        *viewBox = handler->document()->viewBox();
    }

    if (viewBox->isNull())
        return false;

    QStringList pAspectRStrs = pAspectRStr.split(QLatin1String(" "));
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

    const QString orientStr      = attributes.value(QLatin1String("orient")).toString();
    const QString markerUnitsStr = attributes.value(QLatin1String("markerUnits")).toString();

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
        if (orientStr.endsWith(QStringLiteral("turn")))
            a = 360. * toDouble(orientStr.mid(0, orientStr.length()-4), &ok);
        else if (orientStr.endsWith(QStringLiteral("grad")))
            a = toDouble(orientStr.mid(0, orientStr.length()-4), &ok);
        else if (orientStr.endsWith(QStringLiteral("rad")))
            a = 180. / M_PI * toDouble(orientStr.mid(0, orientStr.length()-3), &ok);
        else
            a = toDouble(orientStr, &ok);
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

    QPainterPath qpath;
    qpath.setFillRule(Qt::WindingFill);
    if (!parsePathDataFast(data, qpath, !handler->trustedSourceMode()))
        qCWarning(lcSvgHandler, "Invalid path data; path truncated.");

    QSvgNode *path = new QSvgPath(parent, qpath);
    return path;
}

static QSvgNode *createPolygonNode(QSvgNode *parent,
                                   const QXmlStreamAttributes &attributes,
                                   QSvgHandler *)
{
    QString pointsStr  = attributes.value(QLatin1String("points")).toString();

    //same QPolygon parsing is in createPolylineNode
    const QChar *s = pointsStr.constData();
    QList<qreal> points = parseNumbersList(s);
    QPolygonF poly(points.size()/2);
    for (int i = 0; i < poly.size(); ++i)
        poly[i] = QPointF(points.at(2 * i), points.at(2 * i + 1));
    QSvgNode *polygon = new QSvgPolygon(parent, poly);
    return polygon;
}

static QSvgNode *createPolylineNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &attributes,
                                    QSvgHandler *)
{
    QString pointsStr  = attributes.value(QLatin1String("points")).toString();

    //same QPolygon parsing is in createPolygonNode
    const QChar *s = pointsStr.constData();
    QList<qreal> points = parseNumbersList(s);
    QPolygonF poly(points.size()/2);
    for (int i = 0; i < poly.size(); ++i)
        poly[i] = QPointF(points.at(2 * i), points.at(2 * i + 1));

    QSvgNode *line = new QSvgPolyline(parent, poly);
    return line;
}

static bool parsePrefetchNode(QSvgNode *parent,
                              const QXmlStreamAttributes &attributes,
                              QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return true;
}

static QSvgStyleProperty *createRadialGradientNode(QSvgNode *node,
                                                   const QXmlStreamAttributes &attributes,
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
        ncx = toDouble(cx);
    if (!cy.isEmpty())
        ncy = toDouble(cy);

    qreal nr = 0.5;
    if (!r.isEmpty())
        nr = toDouble(r);
    if (nr <= 0.0)
        return nullptr;

    qreal nfx = ncx;
    if (!fx.isEmpty())
        nfx = toDouble(fx);
    qreal nfy = ncy;
    if (!fy.isEmpty())
        nfy = toDouble(fy);

    QRadialGradient *grad = new QRadialGradient(ncx, ncy, nr, nfx, nfy, 0);
    grad->setInterpolationMode(QGradient::ComponentInterpolation);

    QSvgGradientStyle *prop = new QSvgGradientStyle(grad);
    parseBaseGradient(node, attributes, prop, handler);

    return prop;
}

static QSvgNode *createRectNode(QSvgNode *parent,
                                const QXmlStreamAttributes &attributes,
                                QSvgHandler *handler)
{
    const QStringView x      = attributes.value(QLatin1String("x"));
    const QStringView y      = attributes.value(QLatin1String("y"));
    const QStringView width  = attributes.value(QLatin1String("width"));
    const QStringView height = attributes.value(QLatin1String("height"));
    const QStringView rx      = attributes.value(QLatin1String("rx"));
    const QStringView ry      = attributes.value(QLatin1String("ry"));

    bool ok = true;
    QSvgHandler::LengthType type;
    qreal nwidth = parseLength(width.toString(), &type, handler, &ok);
    if (!ok)
        return nullptr;
    nwidth = convertToPixels(nwidth, true, type);
    qreal nheight = parseLength(height.toString(), &type, handler, &ok);
    if (!ok)
        return nullptr;
    nheight = convertToPixels(nheight, true, type);
    qreal nrx = toDouble(rx);
    qreal nry = toDouble(ry);

    QRectF bounds(toDouble(x), toDouble(y), nwidth, nheight);
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

static QSvgStyleProperty *createSolidColorNode(QSvgNode *parent,
                                               const QXmlStreamAttributes &attributes,
                                               QSvgHandler *handler)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    QStringView solidColorStr = attributes.value(QLatin1String("solid-color"));
    QStringView solidOpacityStr = attributes.value(QLatin1String("solid-opacity"));

    if (solidOpacityStr.isEmpty())
        solidOpacityStr = attributes.value(QLatin1String("opacity"));

    QColor color;
    if (!constructColor(solidColorStr, solidOpacityStr, color, handler))
        return 0;
    QSvgSolidColorStyle *style = new QSvgSolidColorStyle(color);
    return style;
}

static bool parseStopNode(QSvgStyleProperty *parent,
                          const QXmlStreamAttributes &attributes,
                          QSvgHandler *handler)
{
    if (parent->type() != QSvgStyleProperty::GRADIENT)
        return false;
    QString nodeIdStr     = someId(attributes);
    QString xmlClassStr   = attributes.value(QLatin1String("class")).toString();

    //### nasty hack because stop gradients are not in the rendering tree
    //    we force a dummy node with the same id and class into a rendering
    //    tree to figure out whether the selector has a style for it
    //    QSvgStyleSelector should be coded in a way that could avoid it
    QSvgAnimation anim;
    anim.setNodeId(nodeIdStr);
    anim.setXmlClass(xmlClassStr);

    QXmlStreamAttributes xmlAttr = attributes;

#ifndef QT_NO_CSSPARSER
    cssStyleLookup(&anim, handler, handler->selector(), xmlAttr);
#endif
    parseStyle(&anim, xmlAttr, handler);

    QSvgAttributes attrs(xmlAttr, handler);

    QSvgGradientStyle *style =
        static_cast<QSvgGradientStyle*>(parent);
    QStringView colorStr    = attrs.stopColor;
    QColor color;

    bool ok = true;
    qreal offset = convertToNumber(attrs.offset, handler, &ok);
    if (!ok)
        offset = 0.0;
    QString black = QString::fromLatin1("#000000");
    if (colorStr.isEmpty()) {
        colorStr = black;
    }

    constructColor(colorStr, attrs.stopOpacity, color, handler);

    QGradient *grad = style->qgradient();

    offset = qMin(qreal(1), qMax(qreal(0), offset)); // Clamp to range [0, 1]
    QGradientStops stops;
    if (style->gradientStopsSet()) {
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
    style->setGradientStopsSet(true);
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

    QSvgTinyDocument *node = new QSvgTinyDocument(handler->options());
    const QStringView widthStr  = attributes.value(QLatin1String("width"));
    const QStringView heightStr = attributes.value(QLatin1String("height"));
    QString viewBoxStr = attributes.value(QLatin1String("viewBox")).toString();

    QSvgHandler::LengthType type = QSvgHandler::LT_PX; // FIXME: is the default correct?
    qreal width = 0;
    if (!widthStr.isEmpty()) {
        width = parseLength(widthStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            width = convertToPixels(width, true, type);
        node->setWidth(int(width), type == QSvgHandler::LT_PERCENT);
    }
    qreal height = 0;
    if (!heightStr.isEmpty()) {
        height = parseLength(heightStr.toString(), &type, handler);
        if (type != QSvgHandler::LT_PT)
            height = convertToPixels(height, false, type);
        node->setHeight(int(height), type == QSvgHandler::LT_PERCENT);
    }

    QStringList viewBoxValues;
    if (!viewBoxStr.isEmpty()) {
        viewBoxStr = viewBoxStr.replace(QLatin1Char(' '), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\r'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\n'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\t'), QLatin1Char(','));
        viewBoxValues = viewBoxStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (viewBoxValues.size() == 4) {
        QString xStr      = viewBoxValues.at(0).trimmed();
        QString yStr      = viewBoxValues.at(1).trimmed();
        QString widthStr  = viewBoxValues.at(2).trimmed();
        QString heightStr = viewBoxValues.at(3).trimmed();

        QSvgHandler::LengthType lt;
        qreal x = parseLength(xStr, &lt, handler);
        qreal y = parseLength(yStr, &lt, handler);
        qreal w = parseLength(widthStr, &lt, handler);
        qreal h = parseLength(heightStr, &lt, handler);

        node->setViewBox(QRectF(x, y, w, h));

    } else if (width && height) {
        if (type == QSvgHandler::LT_PT) {
            width = convertToPixels(width, false, type);
            height = convertToPixels(height, false, type);
        }
        node->setViewBox(QRectF(0, 0, width, height));
    }
    handler->setDefaultCoordinateSystem(QSvgHandler::LT_PX);

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

    QString viewBoxStr = attributes.value(QLatin1String("viewBox")).toString();

    bool ok = false;
    QSvgHandler::LengthType type;

    qreal nx = parseLength(x.toString(), &type, handler, &ok);
    nx = convertToPixels(nx, true, type);
    if (!ok)
        nx = 0.0;
    else if (type == QSvgHandler::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nx = (nx / 100.) * handler->document()->viewBox().width();
    else if (type == QSvgHandler::LT_PERCENT)
        nx = nx / 100.;

    qreal ny = parseLength(y.toString(), &type, handler, &ok);
    ny = convertToPixels(ny, true, type);
    if (!ok)
        ny = 0.0;
    else if (type == QSvgHandler::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        ny = (ny / 100.) * handler->document()->viewBox().height();
    else if (type == QSvgHandler::LT_PERCENT)
        ny = ny / 100.;

    qreal nwidth = parseLength(width.toString(), &type, handler, &ok);
    nwidth = convertToPixels(nwidth, true, type);
    if (!ok)
        nwidth = 0.0;
    else if (type == QSvgHandler::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nwidth = (nwidth / 100.) * handler->document()->viewBox().width();
    else if (type == QSvgHandler::LT_PERCENT)
        nwidth = nwidth / 100.;

    qreal nheight = parseLength(height.toString(), &type, handler, &ok);
    nheight = convertToPixels(nheight, true, type);
    if (!ok)
        nheight = 0.0;
    else if (type == QSvgHandler::LT_PERCENT && nPatternUnits == QtSvg::UnitTypes::userSpaceOnUse)
        nheight = (nheight / 100.) * handler->document()->viewBox().height();
    else if (type == QSvgHandler::LT_PERCENT)
        nheight = nheight / 100.;


    QStringList viewBoxValues;
    QRectF viewBox;
    if (!viewBoxStr.isEmpty()) {
        viewBoxStr = viewBoxStr.replace(QLatin1Char(' '), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\r'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\n'), QLatin1Char(','));
        viewBoxStr = viewBoxStr.replace(QLatin1Char('\t'), QLatin1Char(','));
        viewBoxValues = viewBoxStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (viewBoxValues.size() == 4) {
        QString xStr      = viewBoxValues.at(0).trimmed();
        QString yStr      = viewBoxValues.at(1).trimmed();
        QString widthStr  = viewBoxValues.at(2).trimmed();
        QString heightStr = viewBoxValues.at(3).trimmed();

        qreal x = convertToNumber(xStr, handler);
        qreal y = convertToNumber(yStr, handler);
        qreal w = convertToNumber(widthStr, handler);
        qreal h = convertToNumber(heightStr, handler);

        if (w > 0 && h > 0)
            viewBox.setRect(x, y, w, h);
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
    QSvgPatternStyle *prop = new QSvgPatternStyle(node);
    node->appendStyleProperty(prop, someId(attributes));

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
                                QSvgHandler *handler)
{
    const QStringView x = attributes.value(QLatin1String("x"));
    const QStringView y = attributes.value(QLatin1String("y"));
    //### editable and rotate not handled
    QSvgHandler::LengthType type;
    qreal nx = parseLength(x.toString(), &type, handler);
    nx = convertToPixels(nx, true, type);
    qreal ny = parseLength(y.toString(), &type, handler);
    ny = convertToPixels(ny, true, type);

    QSvgNode *text = new QSvgText(parent, QPointF(nx, ny));
    return text;
}

static QSvgNode *createTextAreaNode(QSvgNode *parent,
                                    const QXmlStreamAttributes &attributes,
                                    QSvgHandler *handler)
{
    QSvgText *node = static_cast<QSvgText *>(createTextNode(parent, attributes, handler));
    if (node) {
        QSvgHandler::LengthType type;
        qreal width = parseLength(attributes.value(QLatin1String("width")), &type, handler);
        qreal height = parseLength(attributes.value(QLatin1String("height")), &type, handler);
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
    QString linkId = attributes.value(QLatin1String("xlink:href")).toString().remove(0, 1);
    const QStringView xStr = attributes.value(QLatin1String("x"));
    const QStringView yStr = attributes.value(QLatin1String("y"));
    QSvgStructureNode *group = nullptr;

    if (linkId.isEmpty())
        linkId = attributes.value(QLatin1String("href")).toString().remove(0, 1);
    switch (parent->type()) {
    case QSvgNode::Doc:
    case QSvgNode::Defs:
    case QSvgNode::Group:
    case QSvgNode::Switch:
    case QSvgNode::Mask:
        group = static_cast<QSvgStructureNode*>(parent);
        break;
    default:
        break;
    }

    if (group) {
        QPointF pt;
        if (!xStr.isNull() || !yStr.isNull()) {
            QSvgHandler::LengthType type;
            qreal nx = parseLength(xStr.toString(), &type, handler);
            nx = convertToPixels(nx, true, type);

            qreal ny = parseLength(yStr.toString(), &type, handler);
            ny = convertToPixels(ny, true, type);
            pt = QPointF(nx, ny);
        }

        QSvgNode *link = group->scopeNode(linkId);
        if (link) {
            if (parent->isDescendantOf(link))
                qCWarning(lcSvgHandler, "link #%s is recursive!", qPrintable(linkId));

            return new QSvgUse(pt, parent, link);
        }

        //delay link resolving, link might have not been created yet
        return new QSvgUse(pt, parent, linkId);
    }

    qCWarning(lcSvgHandler, "<use> element %s in wrong context!", qPrintable(linkId));
    return 0;
}

static QSvgNode *createVideoNode(QSvgNode *parent,
                                 const QXmlStreamAttributes &attributes,
                                 QSvgHandler *)
{
    Q_UNUSED(parent); Q_UNUSED(attributes);
    return 0;
}

typedef QSvgNode *(*FactoryMethod)(QSvgNode *, const QXmlStreamAttributes &, QSvgHandler *);

static FactoryMethod findGroupFactory(const QString &name, QtSvg::Options options)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = QStringView{name}.mid(1, name.size() - 1);
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

static FactoryMethod findGraphicsFactory(const QString &name, QtSvg::Options options)
{
    Q_UNUSED(options);
    if (name.isEmpty())
        return 0;

    QStringView ref = QStringView{name}.mid(1, name.size() - 1);
    switch (name.at(0).unicode()) {
    case 'a':
        if (ref == QLatin1String("nimation")) return createAnimationNode;
        break;
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

static FactoryMethod findFilterFactory(const QString &name, QtSvg::Options options)
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

    static const QStringList unsupportedFilters = {
        QStringLiteral("feBlend"),
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

typedef bool (*ParseMethod)(QSvgNode *, const QXmlStreamAttributes &, QSvgHandler *);

static ParseMethod findUtilFactory(const QString &name, QtSvg::Options options)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = QStringView{name}.mid(1, name.size() - 1);
    switch (name.at(0).unicode()) {
    case 'a':
        if (ref.isEmpty()) return parseAnchorNode;
        if (ref == QLatin1String("nimate")) return parseAnimateNode;
        if (ref == QLatin1String("nimateColor")) return parseAnimateColorNode;
        if (ref == QLatin1String("nimateMotion")) return parseAimateMotionNode;
        if (ref == QLatin1String("nimateTransform")) return parseAnimateTransformNode;
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

typedef QSvgStyleProperty *(*StyleFactoryMethod)(QSvgNode *,
                                                 const QXmlStreamAttributes &,
                                                 QSvgHandler *);

static StyleFactoryMethod findStyleFactoryMethod(const QString &name)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = QStringView{name}.mid(1, name.size() - 1);
    switch (name.at(0).unicode()) {
    case 'f':
        if (ref == QLatin1String("ont")) return createFontNode;
        break;
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
    return 0;
}

typedef bool (*StyleParseMethod)(QSvgStyleProperty *,
                                 const QXmlStreamAttributes &,
                                 QSvgHandler *);

static StyleParseMethod findStyleUtilFactoryMethod(const QString &name)
{
    if (name.isEmpty())
        return 0;

    QStringView ref = QStringView{name}.mid(1, name.size() - 1);
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
    case 's':
        if (ref == QLatin1String("top")) return parseStopNode;
        break;
    default:
        break;
    }
    return 0;
}

QSvgHandler::QSvgHandler(QIODevice *device, QtSvg::Options options)
    : xml(new QXmlStreamReader(device))
    , m_ownsReader(true)
    , m_options(options)
{
    init();
}

QSvgHandler::QSvgHandler(const QByteArray &data, QtSvg::Options options)
    : xml(new QXmlStreamReader(data))
    , m_ownsReader(true)
    , m_options(options)
{
    init();
}

QSvgHandler::QSvgHandler(QXmlStreamReader *const reader, QtSvg::Options options)
    : xml(reader)
    , m_ownsReader(false)
    , m_options(options)
{
    init();
}

void QSvgHandler::init()
{
    m_doc = 0;
    m_style = 0;
    m_animEnd = 0;
    m_defaultCoords = LT_PX;
    m_defaultPen = QPen(Qt::black, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
    m_defaultPen.setMiterLimit(4);
    parse();
}

static bool detectPatternCycles(const QSvgNode *node, QList<const QSvgNode *> active = {})
{
    QSvgFillStyle *fillStyle = static_cast<QSvgFillStyle*>
        (node->styleProperty(QSvgStyleProperty::FILL));
    if (fillStyle && fillStyle->style() && fillStyle->style()->type() == QSvgStyleProperty::PATTERN) {
        QSvgPatternStyle *patternStyle = static_cast<QSvgPatternStyle *>(fillStyle->style());
        if (active.contains(patternStyle->patternNode()))
            return true;
    }

    QSvgStrokeStyle *strokeStyle = static_cast<QSvgStrokeStyle*>
        (node->styleProperty(QSvgStyleProperty::STROKE));
    if (strokeStyle && strokeStyle->style() && strokeStyle->style()->type() == QSvgStyleProperty::PATTERN) {
        QSvgPatternStyle *patternStyle = static_cast<QSvgPatternStyle *>(strokeStyle->style());
        if (active.contains(patternStyle->patternNode()))
            return true;
    }

    return false;
}

static bool detectCycles(const QSvgNode *node, QList<const QSvgNode *> active = {})
{
    if (Q_UNLIKELY(!node))
        return false;
    switch (node->type()) {
    case QSvgNode::Doc:
    case QSvgNode::Group:
    case QSvgNode::Defs:
    case QSvgNode::Pattern:
    {
        if (node->type() == QSvgNode::Pattern)
            active.append(node);

        auto *g = static_cast<const QSvgStructureNode*>(node);
        for (auto *r : g->renderers()) {
            if (detectCycles(r, active))
                return true;
        }
    }
    break;
    case QSvgNode::Use:
    {
        if (active.contains(node))
            return true;

        auto *u = static_cast<const QSvgUse*>(node);
        auto *target = u->link();
        if (target) {
            active.append(u);
            if (detectCycles(target, active))
                return true;
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
        if (detectPatternCycles(node, active))
            return true;
        break;
    default:
        break;
    }
    return false;
}

// Having too many unfinished elements will cause a stack overflow
// in the dtor of QSvgTinyDocument, see oss-fuzz issue 24000.
static const int unfinishedElementsLimit = 2048;

void QSvgHandler::parse()
{
    xml->setNamespaceProcessing(false);
#ifndef QT_NO_CSSPARSER
    m_selector = new QSvgStyleSelector;
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
            if (remainingUnfinishedElements
                    && startElement(xml->name().toString(), xml->attributes())) {
                --remainingUnfinishedElements;
            } else {
                delete m_doc;
                m_doc = nullptr;
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
            processingInstruction(xml->processingInstructionTarget().toString(), xml->processingInstructionData().toString());
            break;
        default:
            break;
        }
    }
    resolvePaintServers(m_doc);
    resolveNodes();
    if (detectCycles(m_doc)) {
        qCWarning(lcSvgHandler, "Cycles detected in SVG, document discarded.");
        delete m_doc;
        m_doc = nullptr;
    }
}

bool QSvgHandler::startElement(const QString &localName,
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
        const QByteArray msg = '"' + xmlSpace.toString().toLocal8Bit()
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
        node = method(m_doc ? m_nodes.top() : 0, attributes, this);

        if (node) {
            if (!m_doc) {
                Q_ASSERT(node->type() == QSvgNode::Doc);
                m_doc = static_cast<QSvgTinyDocument*>(node);
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
                    QSvgStructureNode *group =
                        static_cast<QSvgStructureNode*>(m_nodes.top());
                    group->addChild(node, someId(attributes));
                }
                    break;
                default:
                    const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
                    qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                    delete node;
                    node = 0;
                    break;
                }
            }

            if (node) {
                parseCoreNode(node, attributes);
#ifndef QT_NO_CSSPARSER
                cssStyleLookup(node, this, m_selector);
#endif
                parseStyle(node, attributes, this);
                if (node->type() == QSvgNode::Filter)
                    m_toBeResolved.append(node);
            }
        }
    } else if (FactoryMethod method = findGraphicsFactory(localName, options())) {
        //rendering element
        Q_ASSERT(!m_nodes.isEmpty());
        node = method(m_nodes.top(), attributes, this);
        if (node) {
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
                if (node->type() == QSvgNode::Tspan) {
                    const QByteArray msg = QByteArrayLiteral("\'tspan\' element in wrong context.");
                    qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                    delete node;
                    node = 0;
                    break;
                }
                QSvgStructureNode *group =
                    static_cast<QSvgStructureNode*>(m_nodes.top());
                group->addChild(node, someId(attributes));
            }
                break;
            case QSvgNode::Text:
            case QSvgNode::Textarea:
                if (node->type() == QSvgNode::Tspan) {
                    static_cast<QSvgText *>(m_nodes.top())->addTspan(static_cast<QSvgTspan *>(node));
                } else {
                    const QByteArray msg = QByteArrayLiteral("\'text\' or \'textArea\' element contains invalid element type.");
                    qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                    delete node;
                    node = 0;
                }
                break;
            default:
                const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
                qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                delete node;
                node = 0;
                break;
            }

            if (node) {
                parseCoreNode(node, attributes);
#ifndef QT_NO_CSSPARSER
                cssStyleLookup(node, this, m_selector);
#endif
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
        }
    } else if (FactoryMethod method = findFilterFactory(localName, options())) {
        //filter nodes to be aded to be filtercontainer
        Q_ASSERT(!m_nodes.isEmpty());
        node = method(m_nodes.top(), attributes, this);
        if (node) {
            if (m_nodes.top()->type() == QSvgNode::Filter ||
                (m_nodes.top()->type() == QSvgNode::FeMerge && node->type() == QSvgNode::FeMergenode)) {
                QSvgStructureNode *container =
                    static_cast<QSvgStructureNode*>(m_nodes.top());
                container->addChild(node, someId(attributes));
            } else {
                const QByteArray msg = QByteArrayLiteral("Could not add child element to parent element because the types are incorrect.");
                qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
                delete node;
                node = 0;
            }
        }
    } else if (ParseMethod method = findUtilFactory(localName, options())) {
        Q_ASSERT(!m_nodes.isEmpty());
        if (!method(m_nodes.top(), attributes, this))
            qCWarning(lcSvgHandler, "%s", msgProblemParsing(localName, xml).constData());
    } else if (StyleFactoryMethod method = findStyleFactoryMethod(localName)) {
        QSvgStyleProperty *prop = method(m_nodes.top(), attributes, this);
        if (prop) {
            m_style = prop;
            m_nodes.top()->appendStyleProperty(prop, someId(attributes));
        } else {
            const QByteArray msg = QByteArrayLiteral("Could not parse node: ") + localName.toLocal8Bit();
            qCWarning(lcSvgHandler, "%s", prefixMessage(msg, xml).constData());
        }
    } else if (StyleParseMethod method = findStyleUtilFactoryMethod(localName)) {
        if (m_style) {
            if (!method(m_style, attributes, this))
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
    else if (m_style && !m_skipNodes.isEmpty() && m_skipNodes.top() != Style)
        m_style = 0;

    return ((localName == QLatin1String("svg")) && (node != Doc));
}

void QSvgHandler::resolvePaintServers(QSvgNode *node, int nestedDepth)
{
    if (!node || (node->type() != QSvgNode::Doc && node->type() != QSvgNode::Group
        && node->type() != QSvgNode::Defs && node->type() != QSvgNode::Switch)) {
        return;
    }

    QSvgStructureNode *structureNode = static_cast<QSvgStructureNode *>(node);

    const QList<QSvgNode *> ren = structureNode->renderers();
    for (auto it = ren.begin(); it != ren.end(); ++it) {
        QSvgFillStyle *fill = static_cast<QSvgFillStyle *>((*it)->styleProperty(QSvgStyleProperty::FILL));
        if (fill && !fill->isPaintStyleResolved()) {
            QString id = fill->paintStyleId();
            QSvgPaintStyleProperty *style = structureNode->styleProperty(id);
            if (style) {
                fill->setFillStyle(style);
            } else {
                qCWarning(lcSvgHandler, "%s", msgCouldNotResolveProperty(id, xml).constData());
                fill->setBrush(Qt::NoBrush);
            }
        }

        QSvgStrokeStyle *stroke = static_cast<QSvgStrokeStyle *>((*it)->styleProperty(QSvgStyleProperty::STROKE));
        if (stroke && !stroke->isPaintStyleResolved()) {
            QString id = stroke->paintStyleId();
            QSvgPaintStyleProperty *style = structureNode->styleProperty(id);
            if (style) {
                stroke->setStyle(style);
            } else {
                qCWarning(lcSvgHandler, "%s", msgCouldNotResolveProperty(id, xml).constData());
                stroke->setStroke(Qt::NoBrush);
            }
        }

        if (nestedDepth < 2048)
            resolvePaintServers(*it, nestedDepth + 1);
    }
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

            QSvgStructureNode *group = static_cast<QSvgStructureNode *>(parent);
            QSvgNode *link = group->scopeNode(useNode->linkId());
            if (!link) {
                qCWarning(lcSvgHandler, "link #%s is undefined!", qPrintable(useNode->linkId()));
                continue;
            }

            if (useNode->parent()->isDescendantOf(link))
                qCWarning(lcSvgHandler, "link #%s is recursive!", qPrintable(useNode->linkId()));

            useNode->setLink(link);
        } else if (node->type() == QSvgNode::Filter) {
            QSvgFilterContainer *filter = static_cast<QSvgFilterContainer *>(node);
            for (const QSvgNode *renderer : filter->renderers()) {
                const QSvgFeFilterPrimitive *primitive = QSvgFeFilterPrimitive::castToFilterPrimitive(renderer);
                if (!primitive || primitive->type() == QSvgNode::FeUnsupported) {
                    filter->setSupported(false);
                    break;
                }
            }
        }
    }
    m_toBeResolved.clear();
}

bool QSvgHandler::characters(const QStringView str)
{
#ifndef QT_NO_CSSPARSER
    if (m_inStyle) {
        QString css = str.toString();
        QCss::StyleSheet sheet;
        QCss::Parser(css).parse(&sheet);
        m_selector->styleSheets.append(sheet);
        return true;
    }
#endif
    if (m_skipNodes.isEmpty() || m_skipNodes.top() == Unknown || m_nodes.isEmpty())
        return true;

    if (m_nodes.top()->type() == QSvgNode::Text || m_nodes.top()->type() == QSvgNode::Textarea) {
        static_cast<QSvgText*>(m_nodes.top())->addText(str.toString());
    } else if (m_nodes.top()->type() == QSvgNode::Tspan) {
        static_cast<QSvgTspan*>(m_nodes.top())->addText(str.toString());
    }

    return true;
}

QIODevice *QSvgHandler::device() const
{
    return xml->device();
}

QSvgTinyDocument *QSvgHandler::document() const
{
    return m_doc;
}

QSvgHandler::LengthType QSvgHandler::defaultCoordinateSystem() const
{
    return m_defaultCoords;
}

void QSvgHandler::setDefaultCoordinateSystem(LengthType type)
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

#ifndef QT_NO_CSSPARSER

void QSvgHandler::setInStyle(bool b)
{
    m_inStyle = b;
}

bool QSvgHandler::inStyle() const
{
    return m_inStyle;
}

QSvgStyleSelector * QSvgHandler::selector() const
{
    return m_selector;
}

#endif // QT_NO_CSSPARSER

bool QSvgHandler::processingInstruction(const QString &target, const QString &data)
{
#ifdef QT_NO_CSSPARSER
    Q_UNUSED(target);
    Q_UNUSED(data);
#else
    if (target == QLatin1String("xml-stylesheet")) {
        QRegularExpression rx(QLatin1String("type=\\\"(.+)\\\""),
                              QRegularExpression::InvertedGreedinessOption);
        QRegularExpressionMatchIterator iter = rx.globalMatch(data);
        bool isCss = false;
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            QString type = match.captured(1);
            if (type.toLower() == QLatin1String("text/css")) {
                isCss = true;
            }
        }

        if (isCss) {
            QRegularExpression rx(QLatin1String("href=\\\"(.+)\\\""),
                                  QRegularExpression::InvertedGreedinessOption);
            QRegularExpressionMatch match = rx.match(data);
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

                QCss::StyleSheet sheet;
                QCss::Parser(css).parse(&sheet);
                m_selector->styleSheets.append(sheet);
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
#ifndef QT_NO_CSSPARSER
    delete m_selector;
    m_selector = 0;
#endif

    if(m_ownsReader)
        delete xml;
}

QT_END_NAMESPACE
