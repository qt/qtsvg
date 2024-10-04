/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qapplication.h>
#include <qdebug.h>
#include <qsvgrenderer.h>
#include <qsvggenerator.h>
#include <QPainter>
#include <QPen>
#include <QPicture>
#include <QXmlStreamReader>

#ifndef SRCDIR
#define SRCDIR
#endif

class tst_QSvgRenderer : public QObject
{
Q_OBJECT

public:
    tst_QSvgRenderer();
    virtual ~tst_QSvgRenderer();

private slots:
    void getSetCheck();
    void inexistentUrl();
    void emptyUrl();
    void invalidUrl_data();
    void invalidUrl();
    void testStrokeWidth();
    void testMapViewBoxToTarget();
    void testRenderElement();
    void testRenderElementToBounds();
    void testRenderDocumentWithSizeToBounds();
    void constructorQXmlStreamReader() const;
    void loadQXmlStreamReader() const;
    void nestedQXmlStreamReader() const;
    void stylePropagation() const;
    void transformForElement() const;
    void boundsOnElement() const;
    void gradientStops() const;
    void gradientRefs();
    void recursiveRefs_data();
    void recursiveRefs();
    void fillRule();
    void opacity();
    void paths();
    void paths2();
    void displayMode();
    void strokeInherit();
    void testFillInheritance();
    void testStopOffsetOpacity();
    void testUseElement();
    void smallFont();
    void styleSheet();
    void duplicateStyleId();
    void oss_fuzz_23731();
    void oss_fuzz_24131();
    void oss_fuzz_24738();
    void imageRendering();
    void illegalAnimateTransform_data();
    void illegalAnimateTransform();

#ifndef QT_NO_COMPRESS
    void testGzLoading();
    void testGzHelper_data();
    void testGzHelper();
#endif

private:
    static const char *const src;
};

const char *const tst_QSvgRenderer::src = "<svg><g><rect x='250' y='250' width='500' height='500'/>"
                                          "<rect id='foo' x='400' y='400' width='100' height='100'/></g></svg>";

tst_QSvgRenderer::tst_QSvgRenderer()
{
}

tst_QSvgRenderer::~tst_QSvgRenderer()
{
}

// Testing get/set functions
void tst_QSvgRenderer::getSetCheck()
{
    QSvgRenderer obj1;
    // int QSvgRenderer::framesPerSecond()
    // void QSvgRenderer::setFramesPerSecond(int)
    obj1.setFramesPerSecond(20);
    QCOMPARE(20, obj1.framesPerSecond());
    obj1.setFramesPerSecond(0);
    QCOMPARE(0, obj1.framesPerSecond());
    obj1.setFramesPerSecond(INT_MIN);
    QCOMPARE(0, obj1.framesPerSecond()); // Can't have a negative framerate
    obj1.setFramesPerSecond(INT_MAX);
    QCOMPARE(INT_MAX, obj1.framesPerSecond());
}

void tst_QSvgRenderer::inexistentUrl()
{
    const char *src = "<svg><g><path d=\"\" style=\"stroke:url(#inexistent)\"/></g></svg>";

    QByteArray data(src);
    QSvgRenderer renderer(data);

    QVERIFY(renderer.isValid());
}

void tst_QSvgRenderer::emptyUrl()
{
    const char *src = "<svg><text fill=\"url()\" /></svg>";

    QByteArray data(src);
    QSvgRenderer renderer(data);

    QVERIFY(renderer.isValid());
}

void tst_QSvgRenderer::invalidUrl_data()
{
    QTest::addColumn<QByteArray>("svg");

    QTest::newRow("01") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url0\" /></svg>");
    QTest::newRow("02") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url(0\" /></svg>");
    QTest::newRow("03") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url (0\" /></svg>");
    QTest::newRow("04") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url ( 0\" /></svg>");
    QTest::newRow("05") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url#\" /></svg>");
    QTest::newRow("06") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url#(\" /></svg>");
    QTest::newRow("07") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url(#\" /></svg>");
    QTest::newRow("08") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url(# \" /></svg>");
    QTest::newRow("09") << QByteArray("<svg><linearGradient id=\"0\"/><circle fill=\"url(# 0\" /></svg>");
    QTest::newRow("10") << QByteArray("<svg><linearGradient id=\"blabla\"/><circle fill=\"urlblabla\" /></svg>");
    QTest::newRow("11") << QByteArray("<svg><linearGradient id=\"blabla\"/><circle fill=\"url(blabla\" /></svg>");
    QTest::newRow("12") << QByteArray("<svg><linearGradient id=\"blabla\"/><circle fill=\"url(blabla)\" /></svg>");
    QTest::newRow("13") << QByteArray("<svg><linearGradient id=\"blabla\"/><circle fill=\"url(#blabla\" /></svg>");
}

void tst_QSvgRenderer::invalidUrl()
{
    QFETCH(QByteArray, svg);

#if QT_CONFIG(regularexpression)
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Could not resolve property"));
#endif
    QSvgRenderer renderer(svg);
    QVERIFY(renderer.isValid());
}

void tst_QSvgRenderer::testStrokeWidth()
{
    qreal squareSize = 30.0;
    qreal strokeWidth = 1.0;
    qreal topLeft = 100.0;

    QSvgGenerator generator;

    QBuffer buffer;
    QByteArray byteArray;
    buffer.setBuffer(&byteArray);
    generator.setOutputDevice(&buffer);

    QPainter painter(&generator);
    painter.setBrush(Qt::blue);

    // Draw a rect with stroke
    painter.setPen(QPen(Qt::black, strokeWidth));
    painter.drawRect(topLeft, topLeft, squareSize, squareSize);

    // Draw a rect without stroke
    painter.setPen(Qt::NoPen);
    painter.drawRect(topLeft, topLeft, squareSize, squareSize);
    painter.end();

    // Insert ID tags into the document
    byteArray.insert(byteArray.indexOf("stroke=\"#000000\""), "id=\"SquareStroke\" ");
    byteArray.insert(byteArray.indexOf("stroke=\"none\""), "id=\"SquareNoStroke\" ");

    QSvgRenderer renderer(byteArray);

    QRectF noStrokeRect = renderer.boundsOnElement("SquareNoStroke");
    QCOMPARE(noStrokeRect.width(), squareSize);
    QCOMPARE(noStrokeRect.height(), squareSize);
    QCOMPARE(noStrokeRect.x(), topLeft);
    QCOMPARE(noStrokeRect.y(), topLeft);

    QRectF strokeRect = renderer.boundsOnElement("SquareStroke");
    QCOMPARE(strokeRect.width(), squareSize + strokeWidth);
    QCOMPARE(strokeRect.height(), squareSize + strokeWidth);
    QCOMPARE(strokeRect.x(), topLeft - (strokeWidth / 2));
    QCOMPARE(strokeRect.y(), topLeft - (strokeWidth / 2));
}

void tst_QSvgRenderer::testMapViewBoxToTarget()
{
    const char *src = "<svg><g><rect x=\"250\" y=\"250\" width=\"500\" height=\"500\" /></g></svg>";
    QByteArray data(src);

    { // No viewport, viewBox, targetRect, or deviceRect -> boundingRect
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter);
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(0, 0, 500, 500));
    }

    { // No viewport, viewBox, targetRect -> deviceRect
        QPicture picture;
        picture.setBoundingRect(QRect(100, 100, 200, 200));
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter);
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(100, 100, 200, 200));
    }

    { // No viewport, viewBox -> targetRect
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QRectF(50, 50, 250, 250));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(50, 50, 250, 250));

    }

    data.replace("<svg>", "<svg viewBox=\"0 0 1000 1000\">");

    { // No viewport, no targetRect -> viewBox
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter);
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(250, 250, 500, 500));
    }

    data.replace("<svg", "<svg width=\"500\" height=\"500\"");

    { // Viewport
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter);
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(125, 125, 250, 250));
    }

    // Requires keep-aspectratio feature
    { // Viewport and viewBox specified -> scale 500x500 square to 1000x750 while preserving aspect ratio gives 750x750
        data = "<svg width=\"1000\" height=\"750\" viewBox=\"-250 -250 500 500\"><g><rect x=\"0\" y=\"0\" width=\"500\" height=\"500\" /></g></svg>";
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.setAspectRatioMode(Qt::KeepAspectRatio);
        rend.render(&painter);
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(500, 375, 750, 750));
    }
}

void tst_QSvgRenderer::testRenderElement()
{
    QByteArray data(src);

    { // No viewport, viewBox, targetRect, or deviceRect -> boundingRect
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QLatin1String("foo"));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));
    }

    { // No viewport, viewBox, targetRect -> deviceRect
        QPicture picture;
        picture.setBoundingRect(QRect(100, 100, 200, 200));
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QLatin1String("foo"));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(100, 100, 200, 200));
    }

    { // No viewport, viewBox -> targetRect
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QLatin1String("foo"), QRectF(50, 50, 250, 250));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(50, 50, 250, 250));

    }

    data.replace("<svg>", "<svg viewBox=\"0 0 1000 1000\">");

    { // No viewport, no targetRect -> view box size
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QLatin1String("foo"));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));
    }

    data.replace("<svg", "<svg width=\"500\" height=\"500\"");

    { // Viewport
        QPicture picture;
        QPainter painter(&picture);
        QSvgRenderer rend(data);
        rend.render(&painter, QLatin1String("foo"));
        painter.end();
        QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));
    }

}

void tst_QSvgRenderer::testRenderElementToBounds()
{
    // QTBUG-79933
    QImage reference(400, 200, QImage::Format_ARGB32);
    {
        reference.fill(Qt::transparent);
        QPainter p(&reference);
        p.fillRect(  0,   0, 200, 100, Qt::blue);
        p.fillRect(200, 100, 200, 100, Qt::blue);
        p.fillRect(  0,   0, 100,  50, Qt::red);
        p.fillRect(100,  50, 100,  50, Qt::red);
        p.fillRect(200, 100, 100,  50, Qt::red);
        p.fillRect(300, 150, 100,  50, Qt::red);
    }

    QImage rendering(400, 200, QImage::Format_ARGB32);
    {
        const char *const src =
                "<svg viewBox=\"0 0 100 100\">" // Presence of a viewBox triggered QTBUG-79933
                "<path id=\"el1\" d=\"M 80,10 H 0 V 0 h 40 v 20 h 40 z\" fill=\"red\" />"
                "<path id=\"el2\" d=\"M 90,100 V 20 h 10 V 60 H 80 v 40 z\" fill=\"blue\" />"
                "</svg>";
        const QByteArray data(src);
        QSvgRenderer rend(data);
        rendering.fill(Qt::transparent);
        QPainter p(&rendering);
        rend.render(&p, "el1", QRectF(  0,   0, 200, 100));
        rend.render(&p, "el2", QRectF(  0,   0, 200, 100));
        rend.render(&p, "el1", QRectF(200, 100, 200, 100));
        rend.render(&p, "el2", QRectF(200, 100, 200, 100));
    }

    QCOMPARE(reference, rendering);
}

void tst_QSvgRenderer::testRenderDocumentWithSizeToBounds()
{
    // QTBUG-80888
    QImage reference(400, 200, QImage::Format_ARGB32);
    {
        reference.fill(Qt::transparent);
        QPainter p(&reference);
        p.fillRect(100, 100, 100,  50, Qt::blue);
        p.fillRect(200,  50, 100,  50, Qt::blue);
    }

    QImage rendering(400, 200, QImage::Format_ARGB32);
    {
        const char *const src = R"src(
        <svg width="20" height="80">
            <g transform="translate(-100,-100)">
                <path d="m 110,180 v -80 h 10 v 40 h -20 v 40 z" fill="blue" />
            </g>
        </svg>
        )src";
        const QByteArray data(src);
        QSvgRenderer rend(data);
        rendering.fill(Qt::transparent);
        QPainter p(&rendering);
        rend.render(&p, QRectF(100, 50, 200, 100));
    }

    QCOMPARE(reference, rendering);
}

void tst_QSvgRenderer::constructorQXmlStreamReader() const
{
    const QByteArray data(src);

    QXmlStreamReader reader(data);

    QPicture picture;
    QPainter painter(&picture);
    QSvgRenderer rend(&reader);
    rend.render(&painter, QLatin1String("foo"));
    painter.end();
    QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));
}

void tst_QSvgRenderer::loadQXmlStreamReader() const
{
    const QByteArray data(src);

    QXmlStreamReader reader(data);
    QPicture picture;
    QPainter painter(&picture);
    QSvgRenderer rend;
    rend.load(&reader);
    rend.render(&painter, QLatin1String("foo"));
    painter.end();
    QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));
}


void tst_QSvgRenderer::nestedQXmlStreamReader() const
{
    const QByteArray data(QByteArray("<bar>") + QByteArray(src) + QByteArray("</bar>"));

    QXmlStreamReader reader(data);

    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QLatin1String("bar"));

    QPicture picture;
    QPainter painter(&picture);
    QSvgRenderer renderer(&reader);
    renderer.render(&painter, QLatin1String("foo"));
    painter.end();
    QCOMPARE(picture.boundingRect(), QRect(0, 0, 100, 100));

    QCOMPARE(reader.readNext(), QXmlStreamReader::EndElement);
    QCOMPARE(reader.name().toString(), QLatin1String("bar"));
    QCOMPARE(reader.readNext(), QXmlStreamReader::EndDocument);

    QVERIFY(reader.atEnd());
    QVERIFY(!reader.hasError());
}

void tst_QSvgRenderer::stylePropagation() const
{
    QByteArray data("<svg>"
                      "<g id='foo' style='fill:#ffff00;'>"
                        "<g id='bar' style='fill:#ff00ff;'>"
                          "<g id='baz' style='fill:#00ffff;'>"
                            "<rect id='alpha' x='0' y='0' width='100' height='100'/>"
                          "</g>"
                          "<rect id='beta' x='100' y='0' width='100' height='100'/>"
                        "</g>"
                        "<rect id='gamma' x='0' y='100' width='100' height='100'/>"
                      "</g>"
                      "<rect id='delta' x='100' y='100' width='100' height='100'/>"
                    "</svg>"); // alpha=cyan, beta=magenta, gamma=yellow, delta=black

    QImage image1(200, 200, QImage::Format_RGB32);
    QImage image2(200, 200, QImage::Format_RGB32);
    QImage image3(200, 200, QImage::Format_RGB32);
    QPainter painter;
    QSvgRenderer renderer(data);
    QLatin1String parts[4] = {QLatin1String("alpha"), QLatin1String("beta"), QLatin1String("gamma"), QLatin1String("delta")};

    QVERIFY(painter.begin(&image1));
    for (int i = 0; i < 4; ++i)
        renderer.render(&painter, parts[i], QRectF(renderer.boundsOnElement(parts[i])));
    painter.end();

    QVERIFY(painter.begin(&image2));
    renderer.render(&painter, renderer.viewBoxF());
    painter.end();

    QVERIFY(painter.begin(&image3));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::cyan));
    painter.drawRect(0, 0, 100, 100);
    painter.setBrush(QBrush(Qt::magenta));
    painter.drawRect(100, 0, 100, 100);
    painter.setBrush(QBrush(Qt::yellow));
    painter.drawRect(0, 100, 100, 100);
    painter.setBrush(QBrush(Qt::black));
    painter.drawRect(100, 100, 100, 100);
    painter.end();

    QCOMPARE(image1, image2);
    QCOMPARE(image1, image3);
}

static qreal transformNorm(const QTransform &m)
{
    return qSqrt(m.m11() * m.m11()
        + m.m12() * m.m12()
        + m.m13() * m.m13()
        + m.m21() * m.m21()
        + m.m22() * m.m22()
        + m.m23() * m.m23()
        + m.m31() * m.m31()
        + m.m32() * m.m32()
        + m.m33() * m.m33());
}

template<typename T>
static inline bool diffIsSmallEnough(T diff, T norm)
{
    static_assert(std::is_same_v<T, double> || std::is_same_v<T, float>);
    T sigma = []{
        if constexpr (std::is_same_v<T, double>)
            return 1e-12;
        else
            return 1e-5;
    }();
    return diff <= sigma * norm;
}

static void compareTransforms(const QTransform &m1, const QTransform &m2)
{
    qreal norm1 = transformNorm(m1);
    qreal norm2 = transformNorm(m2);
    qreal diffNorm = transformNorm(QTransform(m1.m11() - m2.m11(),
                                              m1.m12() - m2.m12(),
                                              m1.m13() - m2.m13(),
                                              m1.m21() - m2.m21(),
                                              m1.m22() - m2.m22(),
                                              m1.m23() - m2.m23(),
                                              m1.m31() - m2.m31(),
                                              m1.m32() - m2.m32(),
                                              m1.m33() - m2.m33()));
    QVERIFY(diffIsSmallEnough(diffNorm, qMin(norm1, norm2)));
}

void tst_QSvgRenderer::transformForElement() const
{
    QByteArray data("<svg>"
                      "<g id='ichi' transform='translate(-3,1)'>"
                        "<g id='ni' transform='rotate(45)'>"
                          "<g id='san' transform='scale(4,2)'>"
                            "<g id='yon' transform='matrix(1,2,3,4,5,6)'>"
                              "<rect id='firkant' x='-1' y='-1' width='2' height='2'/>"
                            "</g>"
                          "</g>"
                        "</g>"
                      "</g>"
                    "</svg>");

    QImage image(13, 37, QImage::Format_RGB32);
    QPainter painter(&image);
    QSvgRenderer renderer(data);

    compareTransforms(painter.worldTransform(), renderer.transformForElement(QLatin1String("ichi")));
    painter.translate(-3, 1);
    compareTransforms(painter.worldTransform(), renderer.transformForElement(QLatin1String("ni")));
    painter.rotate(45);
    compareTransforms(painter.worldTransform(), renderer.transformForElement(QLatin1String("san")));
    painter.scale(4, 2);
    compareTransforms(painter.worldTransform(), renderer.transformForElement(QLatin1String("yon")));
    painter.setWorldTransform(QTransform(1, 2, 3, 4, 5, 6), true);
    compareTransforms(painter.worldTransform(), renderer.transformForElement(QLatin1String("firkant")));
}

void tst_QSvgRenderer::boundsOnElement() const
{
    QByteArray data("<svg>"
                      "<g id=\"prim\" transform=\"translate(-3,1)\">"
                        "<g id=\"sjokade\" stroke=\"none\" stroke-width=\"10\">"
                          "<rect id=\"kaviar\" transform=\"rotate(45)\" x=\"-10\" y=\"-10\" width=\"20\" height=\"20\"/>"
                        "</g>"
                        "<g id=\"nugatti\" stroke=\"black\" stroke-width=\"2\">"
                          "<use x=\"0\" y=\"0\" transform=\"rotate(45)\" xlink:href=\"#kaviar\"/>"
                        "</g>"
                        "<g id=\"nutella\" stroke=\"none\" stroke-width=\"10\">"
                          "<path id=\"baconost\" transform=\"rotate(45)\" d=\"M-10 -10 L10 -10 L10 10 L-10 10 Z\"/>"
                        "</g>"
                        "<g id=\"hapaa\" transform=\"translate(-2,2)\" stroke=\"black\" stroke-width=\"2\">"
                          "<use x=\"0\" y=\"0\" transform=\"rotate(45)\" xlink:href=\"#baconost\"/>"
                        "</g>"
                      "</g>"
                      "<text id=\"textA\" x=\"50\" y=\"100\">Lorem ipsum</text>"
                      "<text id=\"textB\" transform=\"matrix(1 0 0 1 50 100)\">Lorem ipsum</text>"
                      "<g id=\"textGroup\">"
                        "<text id=\"textC\" transform=\"matrix(1 0 0 2 20 10)\">Lorem ipsum</text>"
                        "<text id=\"textD\" transform=\"matrix(1 0 0 2 30 40)\">Lorem ipsum</text>"
                      "</g>"
                    "</svg>");
    
    qreal sqrt2 = qSqrt(2);
    QSvgRenderer renderer(data);
    QCOMPARE(renderer.boundsOnElement(QLatin1String("sjokade")), QRectF(-10 * sqrt2, -10 * sqrt2, 20 * sqrt2, 20 * sqrt2));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("kaviar")), QRectF(-10 * sqrt2, -10 * sqrt2, 20 * sqrt2, 20 * sqrt2));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("nugatti")), QRectF(-11, -11, 22, 22));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("nutella")), QRectF(-10 * sqrt2, -10 * sqrt2, 20 * sqrt2, 20 * sqrt2));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("baconost")), QRectF(-10 * sqrt2, -10 * sqrt2, 20 * sqrt2, 20 * sqrt2));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("hapaa")), QRectF(-13, -9, 22, 22));
    QCOMPARE(renderer.boundsOnElement(QLatin1String("prim")), QRectF(-10 * sqrt2 - 3, -10 * sqrt2 + 1, 20 * sqrt2, 20 * sqrt2));

    QRectF textBoundsA = renderer.boundsOnElement(QLatin1String("textA"));
    QVERIFY(!textBoundsA.isEmpty());
    QCOMPARE(renderer.boundsOnElement(QLatin1String("textB")), textBoundsA);

    QRect cBounds = renderer.boundsOnElement(QLatin1String("textC")).toRect();
    QRect dBounds = renderer.boundsOnElement(QLatin1String("textD")).toRect();
    QVERIFY(!cBounds.isEmpty());
    QCOMPARE(cBounds.size(), dBounds.size());
    QRect groupBounds = renderer.boundsOnElement(QLatin1String("textGroup")).toRect();
    QCOMPARE(groupBounds, cBounds | dBounds);
}

void tst_QSvgRenderer::gradientStops() const
{
    {
        QByteArray data("<svg>"
                          "<defs>"
                            "<linearGradient id=\"gradient\">"
                            "</linearGradient>"
                          "</defs>"
                          "<rect fill=\"url(#gradient)\" height=\"64\" width=\"64\" x=\"0\" y=\"0\"/>"
                        "</svg>");
        QSvgRenderer renderer(data);

        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied), refImage(64, 64, QImage::Format_ARGB32_Premultiplied);
        image.fill(0x87654321);
        refImage.fill(0x87654321);

        QPainter painter(&image);
        renderer.render(&painter);
        QCOMPARE(image, refImage);
    }

    {
        QByteArray data("<svg>"
                          "<defs>"
                            "<linearGradient id=\"gradient\">"
                              "<stop offset=\"1\" stop-color=\"cyan\"/>"
                            "</linearGradient>"
                          "</defs>"
                          "<rect fill=\"url(#gradient)\" height=\"64\" width=\"64\" x=\"0\" y=\"0\"/>"
                        "</svg>");
        QSvgRenderer renderer(data);

        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied), refImage(64, 64, QImage::Format_ARGB32_Premultiplied);
        refImage.fill(0xff00ffff);

        QPainter painter(&image);
        renderer.render(&painter);
        QCOMPARE(image, refImage);
    }

    {
        QByteArray data("<svg>"
                          "<defs>"
                            "<linearGradient id=\"gradient\">"
                              "<stop offset=\"0\" stop-color=\"red\"/>"
                              "<stop offset=\"0\" stop-color=\"cyan\"/>"
                              "<stop offset=\"0.5\" stop-color=\"cyan\"/>"
                              "<stop offset=\"0.5\" stop-color=\"magenta\"/>"
                              "<stop offset=\"0.5\" stop-color=\"yellow\"/>"
                              "<stop offset=\"1\" stop-color=\"yellow\"/>"
                              "<stop offset=\"1\" stop-color=\"blue\"/>"
                            "</linearGradient>"
                          "</defs>"
                          "<rect fill=\"url(#gradient)\" height=\"64\" width=\"64\" x=\"0\" y=\"0\"/>"
                        "</svg>");
        QSvgRenderer renderer(data);

        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied), refImage(64, 64, QImage::Format_ARGB32_Premultiplied);

        QPainter painter;
        painter.begin(&refImage);
        painter.fillRect(QRectF(0, 0, 32, 64), Qt::cyan);
        painter.fillRect(QRectF(32, 0, 32, 64), Qt::yellow);
        painter.end();

        painter.begin(&image);
        renderer.render(&painter);
        painter.end();

        QCOMPARE(image, refImage);
    }
}

void tst_QSvgRenderer::gradientRefs()
{
    const char *svgs[] = {
        "<svg>"
            "<defs>"
                "<linearGradient id=\"gradient\">"
                    "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0\"/>"
                    "<stop offset=\"1\" stop-color=\"blue\"/>"
                "</linearGradient>"
            "</defs>"
            "<rect fill=\"url(#gradient)\" height=\"8\" width=\"256\" x=\"0\" y=\"0\"/>"
        "</svg>",
        "<svg>"
            "<defs>"
                "<linearGradient id=\"gradient\" xlink:href=\"#gradient0\">"
                "</linearGradient>"
                "<linearGradient id=\"gradient0\">"
                    "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0\"/>"
                    "<stop offset=\"1\" stop-color=\"blue\"/>"
                "</linearGradient>"
            "</defs>"
            "<rect fill=\"url(#gradient)\" height=\"8\" width=\"256\" x=\"0\" y=\"0\"/>"
        "</svg>",
        "<svg>"
            "<defs>"
                "<linearGradient id=\"gradient0\">"
                    "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0\"/>"
                    "<stop offset=\"1\" stop-color=\"blue\"/>"
                "</linearGradient>"
                "<linearGradient id=\"gradient\" xlink:href=\"#gradient0\">"
                "</linearGradient>"
            "</defs>"
            "<rect fill=\"url(#gradient)\" height=\"8\" width=\"256\" x=\"0\" y=\"0\"/>"
        "</svg>",
        "<svg>"
            "<rect fill=\"url(#gradient)\" height=\"8\" width=\"256\" x=\"0\" y=\"0\"/>"
            "<defs>"
                "<linearGradient id=\"gradient0\">"
                    "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0\"/>"
                    "<stop offset=\"1\" stop-color=\"blue\"/>"
                "</linearGradient>"
                "<linearGradient id=\"gradient\" xlink:href=\"#gradient0\">"
                "</linearGradient>"
            "</defs>"
        "</svg>",
        "<svg>"
            "<defs>"
                "<linearGradient xlink:href=\"#0\" id=\"0\">"
                    "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0\"/>"
                    "<stop offset=\"1\" stop-color=\"blue\"/>"
                "</linearGradient>"
            "</defs>"
            "<rect fill=\"url(#0)\" height=\"8\" width=\"256\" x=\"0\" y=\"0\"/>"
        "</svg>"
    };
    for (size_t i = 0 ; i < sizeof(svgs) / sizeof(svgs[0]) ; ++i)
    {
        QByteArray data = svgs[i];
        QSvgRenderer renderer(data);

        QImage image(256, 8, QImage::Format_ARGB32_Premultiplied);
        image.fill(0);

        QPainter painter(&image);
        renderer.render(&painter);

        const QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(3));
        QRgb left = line[0]; // transparent black
        QRgb mid = line[127]; // semi transparent magenta
        QRgb right = line[255]; // opaque blue

        QVERIFY((qAlpha(left) < 3) && (qRed(left) < 3) && (qGreen(left) == 0) && (qBlue(left) < 3));
        QVERIFY((qAbs(qAlpha(mid) - 127) < 3) && (qAbs(qRed(mid) - 63) < 4) && (qGreen(mid) == 0) && (qAbs(qBlue(mid) - 63) < 4));
        QVERIFY((qAlpha(right) > 253) && (qRed(right) < 3) && (qGreen(right) == 0) && (qBlue(right) > 251));
    }
}

void tst_QSvgRenderer::recursiveRefs_data()
{
    QTest::addColumn<QByteArray>("svg");

    QTest::newRow("single") << QByteArray("<svg>"
                                          "<linearGradient id='0' xlink:href='#0'/>"
                                          "<rect x='0' y='0' width='20' height='20' fill='url(#0)'/>"
                                          "</svg>");

    QTest::newRow("double") << QByteArray("<svg>"
                                          "<linearGradient id='0' xlink:href='#1'/>"
                                          "<linearGradient id='1' xlink:href='#0'/>"
                                          "<rect x='0' y='0' width='20' height='20' fill='url(#0)'/>"
                                          "</svg>");

    QTest::newRow("triple") << QByteArray("<svg>"
                                          "<linearGradient id='0' xlink:href='#1'/>"
                                          "<linearGradient id='1' xlink:href='#2'/>"
                                          "<linearGradient id='2' xlink:href='#0'/>"
                                          "<rect x='0' y='0' width='20' height='20' fill='url(#0)'/>"
                                          "</svg>");
}

void tst_QSvgRenderer::recursiveRefs()
{
    QFETCH(QByteArray, svg);

    QImage image(20, 20, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::green);
    QImage refImage = image.copy();

    QSvgRenderer renderer(svg);
    QPainter painter(&image);
    renderer.render(&painter);
    QCOMPARE(image, refImage);
}


#ifndef QT_NO_COMPRESS
void tst_QSvgRenderer::testGzLoading()
{
    QSvgRenderer renderer(QFINDTESTDATA("heart.svgz"));
    QVERIFY(renderer.isValid());

    QSvgRenderer resourceRenderer(QLatin1String(":/heart.svgz"));
    QVERIFY(resourceRenderer.isValid());

    QFile largeFileGz(QFINDTESTDATA("large.svgz"));
    largeFileGz.open(QIODevice::ReadOnly);
    QByteArray data = largeFileGz.readAll();
    QSvgRenderer autoDetectGzData(data);
    QVERIFY(autoDetectGzData.isValid());
}

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
QByteArray qt_inflateGZipDataFrom(QIODevice *device);
QT_END_NAMESPACE
#endif

void tst_QSvgRenderer::testGzHelper_data()
{
    QTest::addColumn<QByteArray>("in");
    QTest::addColumn<QByteArray>("out");

    QTest::newRow("empty") << QByteArray() << QByteArray();
    QTest::newRow("small") << QByteArray::fromHex(QByteArray("1f8b08005819934800034b"
            "cbcfe70200a865327e04000000")) << QByteArray("foo\n");

    QFile largeFileGz(QFINDTESTDATA("large.svgz"));
    largeFileGz.open(QIODevice::ReadOnly);
    QFile largeFile(QFINDTESTDATA("large.svg"));
    largeFile.open(QIODevice::ReadOnly);
    QTest::newRow("large") << largeFileGz.readAll() << largeFile.readAll();

    QTest::newRow("zeroes") << QByteArray::fromHex(QByteArray("1f8b0800131f9348000333"
            "301805a360148c54c00500d266708601040000")) << QByteArray(1024, '0').append('\n');

    QTest::newRow("twoMembers") << QByteArray::fromHex(QByteArray("1f8b08001c2a934800034b"
            "cbcfe70200a865327e040000001f8b08001c2a934800034b4a2ce20200e9b3a20404000000"))
        << QByteArray("foo\nbar\n");

    QTest::newRow("corruptedSecondMember") << QByteArray::fromHex(QByteArray("1f8b08001c2a934800034b"
            "cbcfe70200a865327e040000001f8c08001c2a934800034b4a2ce20200e9b3a20404000000"))
        << QByteArray();

}

void tst_QSvgRenderer::testGzHelper()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QByteArray, in);
    QFETCH(QByteArray, out);

    QBuffer buffer(&in);
    buffer.open(QIODevice::ReadOnly);
    QVERIFY(buffer.isReadable());
    QByteArray result = qt_inflateGZipDataFrom(&buffer);
    QCOMPARE(result, out);
#endif
}
#endif

void tst_QSvgRenderer::fillRule()
{
    static const char *svgs[] = {
        // Paths
        // Default fill-rule (nonzero)
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <path d=\"M10 0 L30 0 L30 30 L0 30 L0 10 L20 10 L20 20 L10 20 Z\" fill=\"red\" />"
        "</svg>",
        // nonzero
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <path d=\"M10 0 L30 0 L30 30 L0 30 L0 10 L20 10 L20 20 L10 20 Z\" fill=\"red\" fill-rule=\"nonzero\" />"
        "</svg>",
        // evenodd
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <path d=\"M10 0 L30 0 L30 30 L0 30 L0 10 L20 10 L20 20 L10 20 Z\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>",

        // Polygons
        // Default fill-rule (nonzero)
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <polygon points=\"10 0 30 0 30 30 0 30 0 10 20 10 20 20 10 20\" fill=\"red\" />"
        "</svg>",
        // nonzero
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <polygon points=\"10 0 30 0 30 30 0 30 0 10 20 10 20 20 10 20\" fill=\"red\" fill-rule=\"nonzero\" />"
        "</svg>",
        // evenodd
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"30\" width=\"30\" fill=\"blue\" />"
        "   <polygon points=\"10 0 30 0 30 30 0 30 0 10 20 10 20 20 10 20\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>"
    };

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage refImageNonZero(30, 30, QImage::Format_ARGB32_Premultiplied);
    QImage refImageEvenOdd(30, 30, QImage::Format_ARGB32_Premultiplied);
    refImageNonZero.fill(0xffff0000);
    refImageEvenOdd.fill(0xffff0000);
    QPainter p;
    p.begin(&refImageNonZero);
    p.fillRect(QRectF(0, 0, 10, 10), Qt::blue);
    p.end();
    p.begin(&refImageEvenOdd);
    p.fillRect(QRectF(0, 0, 10, 10), Qt::blue);
    p.fillRect(QRectF(10, 10, 10, 10), Qt::blue);
    p.end();

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QImage image(30, 30, QImage::Format_ARGB32_Premultiplied);
        image.fill(0);
        p.begin(&image);
        renderer.render(&p);
        p.end();
        QCOMPARE(image, i % 3 == 2 ? refImageEvenOdd : refImageNonZero);
    }
}

static void opacity_drawSvgAndVerify(const QByteArray &data)
{
    QSvgRenderer renderer(data);
    QVERIFY(renderer.isValid());
    QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
    image.fill(0xffff00ff);
    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();
    QCOMPARE(image.pixel(5, 5), 0xff7f7f7f);
}

void tst_QSvgRenderer::opacity()
{
    static const char *opacities[] = {"-1.4641", "0", "0.5", "1", "1.337"};
    static const char *firstColors[] = {"#7f7f7f", "#7f7f7f", "#402051", "blue", "#123456"};
    static const char *secondColors[] = {"red", "#bad", "#bedead", "#7f7f7f", "#7f7f7f"};

    // Fill-opacity
    for (int i = 0; i < 5; ++i) {
        QByteArray data("<svg><rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"");
        data.append(firstColors[i]);
        data.append("\"/><rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"");
        data.append(secondColors[i]);
        data.append("\" fill-opacity=\"");
        data.append(opacities[i]);
        data.append("\"/></svg>");
        opacity_drawSvgAndVerify(data);
    }
    // Stroke-opacity
    for (int i = 0; i < 5; ++i) {
        QByteArray data("<svg viewBox=\"0 0 10 10\"><polyline points=\"0 5 10 5\" fill=\"none\" stroke=\"");
        data.append(firstColors[i]);
        data.append("\" stroke-width=\"10\"/><line x1=\"5\" y1=\"0\" x2=\"5\" y2=\"10\" fill=\"none\" stroke=\"");
        data.append(secondColors[i]);
        data.append("\" stroke-width=\"10\" stroke-opacity=\"");
        data.append(opacities[i]);
        data.append("\"/></svg>");
        opacity_drawSvgAndVerify(data);
    }
    // As gradients:
    // Fill-opacity
    for (int i = 0; i < 5; ++i) {
        QByteArray data("<svg><defs><linearGradient id=\"gradient\"><stop offset=\"0\" stop-color=\"");
        data.append(secondColors[i]);
        data.append("\"/><stop offset=\"1\" stop-color=\"");
        data.append(secondColors[i]);
        data.append("\"/></linearGradient></defs><rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"");
        data.append(firstColors[i]);
        data.append("\"/><rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"url(#gradient)\" fill-opacity=\"");
        data.append(opacities[i]);
        data.append("\"/></svg>");
        opacity_drawSvgAndVerify(data);
    }
    // Stroke-opacity
    for (int i = 0; i < 5; ++i) {
        QByteArray data("<svg viewBox=\"0 0 10 10\"><defs><linearGradient id=\"grad\"><stop offset=\"0\" stop-color=\"");
        data.append(secondColors[i]);
        data.append("\"/><stop offset=\"1\" stop-color=\"");
        data.append(secondColors[i]);
        data.append("\"/></linearGradient></defs><polyline points=\"0 5 10 5\" fill=\"none\" stroke=\"");
        data.append(firstColors[i]);
        data.append("\" stroke-width=\"10\" /><line x1=\"5\" y1=\"0\" x2=\"5\" y2=\"10\" fill=\"none\" stroke=\"url(#grad)\" stroke-width=\"10\" stroke-opacity=\"");
        data.append(opacities[i]);
        data.append("\" /></svg>");
        opacity_drawSvgAndVerify(data);
    }
}

void tst_QSvgRenderer::paths()
{
    static const char *svgs[] = {
        // Absolute coordinates, explicit commands.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\"M50 0 V50 H0 Q0 25 25 25 T50 0 C25 0 50 50 25 50 S25 0 0 0 Z\" fill=\"red\" fill-rule=\"evenodd\"/>"
        "</svg>",
        // Absolute coordinates, implicit commands.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\"M50 0 50 50 0 50 Q0 25 25 25 Q50 25 50 0 C25 0 50 50 25 50 C0 50 25 0 0 0 Z\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>",
        // Relative coordinates, explicit commands.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\"m50 0 v50 h-50 q0 -25 25 -25 t25 -25 c-25 0 0 50 -25 50 s0 -50 -25 -50 z\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>",
        // Relative coordinates, implicit commands.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\"m50 0 0 50 -50 0 q0 -25 25 -25 25 0 25 -25 c-25 0 0 50 -25 50 -25 0 0 -50 -25 -50 z\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>",
        // Absolute coordinates, explicit commands, minimal whitespace.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\"m50 0v50h-50q0-25 25-25t25-25c-25 0 0 50-25 50s0-50-25-50z\" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>",
        // Absolute coordinates, explicit commands, extra whitespace.
        "<svg>"
        "   <rect x=\"0\" y=\"0\" height=\"50\" width=\"50\" fill=\"blue\" />"
        "   <path d=\" M  50  0  V  50  H  0  Q 0  25   25 25 T  50 0 C 25   0 50  50 25 50 S  25 0 0  0 Z  \" fill=\"red\" fill-rule=\"evenodd\" />"
        "</svg>"
    };

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QVERIFY(renderer.isValid());
        images[i] = QImage(50, 50, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(0);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
        if (i != 0) {
            QCOMPARE(images[i], images[0]);
        }
    }
}

void tst_QSvgRenderer::paths2()
{
    const char *svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\">"
            "<path d=\"M 3 8 A 5 5 0 1013 8\" id=\"path1\"/>"
        "</svg>";

    QByteArray data(svg);
    QSvgRenderer renderer(data);
    QVERIFY(renderer.isValid());
    QCOMPARE(renderer.boundsOnElement(QLatin1String("path1")).toRect(), QRect(3, 8, 10, 5));
}

void tst_QSvgRenderer::displayMode()
{
    static const char *svgs[] = {
        // All visible.
        "<svg>"
        "   <g>"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" />"
        "   </g>"
        "</svg>",
        // Don't display svg element.
        "<svg display=\"none\">"
        "   <g>"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" />"
        "   </g>"
        "</svg>",
        // Don't display g element.
        "<svg>"
        "   <g display=\"none\">"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" />"
        "   </g>"
        "</svg>",
        // Don't display first rect element.
        "<svg>"
        "   <g>"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" display=\"none\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" />"
        "   </g>"
        "</svg>",
        // Don't display second rect element.
        "<svg>"
        "   <g>"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" display=\"none\" />"
        "   </g>"
        "</svg>",
        // Don't display svg element, but set display mode to "inline" for other elements.
        "<svg display=\"none\">"
        "   <g display=\"inline\">"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"red\" display=\"inline\" />"
        "       <rect x=\"0\" y=\"0\" height=\"10\" width=\"10\" fill=\"blue\" display=\"inline\" />"
        "   </g>"
        "</svg>"
    };

    QRgb expectedColors[] = {0xff0000ff, 0xff00ff00, 0xff00ff00, 0xff0000ff, 0xffff0000, 0xff00ff00};

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QVERIFY(renderer.isValid());
        QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
        image.fill(0xff00ff00);
        p.begin(&image);
        renderer.render(&p);
        p.end();
        QCOMPARE(image.pixel(5, 5), expectedColors[i]);
    }
}

void tst_QSvgRenderer::strokeInherit()
{
    static const char *svgs[] = {
        // Reference.
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\"/>"
        "   </g>"
        "</svg>",
        // stroke
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"none\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke=\"blue\"/>"
        "   </g>"
        "   <g stroke=\"yellow\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\" stroke=\"green\"/>"
        "   </g>"
        "</svg>",
        // stroke-width
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"0\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-width=\"20\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"10\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\" stroke-width=\"0\"/>"
        "   </g>"
        "</svg>",
        // stroke-linecap
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"round\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-linecap=\"butt\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\"/>"
        "   </g>"
        "</svg>",
        // stroke-linejoin
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"round\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-linejoin=\"miter\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\"/>"
        "   </g>"
        "</svg>",
        // stroke-miterlimit
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"2\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-miterlimit=\"1\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\"/>"
        "   </g>"
        "</svg>",
        // stroke-dasharray
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"1,1,1,1,1,1,3,1,3,1,3,1,1,1,1,1,1,3\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-dasharray=\"20,10\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"none\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\" stroke-dasharray=\"3,3,1\"/>"
        "   </g>"
        "</svg>",
        // stroke-dashoffset
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"0\" stroke-opacity=\"0.5\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-dashoffset=\"10\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"0\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\" stroke-dashoffset=\"4.5\"/>"
        "   </g>"
        "</svg>",
        // stroke-opacity
        "<svg viewBox=\"0 0 200 30\">"
        "   <g stroke=\"blue\" stroke-width=\"20\" stroke-linecap=\"butt\""
        "       stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" stroke-dasharray=\"20,10\""
        "       stroke-dashoffset=\"10\" stroke-opacity=\"0\">"
        "       <polyline fill=\"none\" points=\"10 10 100 10 100 20 190 20\" stroke-opacity=\"0.5\"/>"
        "   </g>"
        "   <g stroke=\"green\" stroke-width=\"0\" stroke-dasharray=\"3,3,1\" stroke-dashoffset=\"4.5\">"
        "       <polyline fill=\"none\" points=\"10 25 80 25\"/>"
        "   </g>"
        "</svg>"
    };

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QVERIFY(renderer.isValid());
        images[i] = QImage(200, 30, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
        if (i != 0) {
            QCOMPARE(images[0], images[i]);
        }
    }
}

void tst_QSvgRenderer::testFillInheritance()
{
    static const char *svgs[] = {
        //reference
        "<svg viewBox = \"0 0 200 200\">"
        "    <polygon points=\"20,20 50,120 100,10 40,80 50,80\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.5\" fill-rule = \"evenodd\"/>"
        "</svg>",
        "<svg viewBox = \"0 0 200 200\">"
        "    <polygon points=\"20,20 50,120 100,10 40,80 50,80\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.5\" fill-rule = \"evenodd\"/>"
        "    <rect x = \"40\" y = \"40\" width = \"70\" height =\"20\" fill = \"green\" fill-opacity = \"0\"/>"
        "</svg>",
        "<svg viewBox = \"0 0 200 200\">"
        "   <g fill = \"red\" fill-opacity = \"0.5\" fill-rule = \"evenodd\">"
        "       <polygon points=\"20,20 50,120 100,10 40,80 50,80\" stroke = \"blue\"/>"
        "   </g>"
        "    <rect x = \"40\" y = \"40\" width = \"70\" height =\"20\" fill = \"green\" fill-opacity = \"0\"/>"
        "</svg>",
        "<svg viewBox = \"0 0 200 200\">"
        "   <g  fill = \"green\" fill-rule = \"nonzero\">"
        "       <polygon points=\"20,20 50,120 100,10 40,80 50,80\" stroke = \"blue\" fill = \"red\" fill-opacity = \"0.5\" fill-rule = \"evenodd\"/>"
        "   </g>"
        "   <g fill-opacity = \"0.8\" fill = \"red\">"
        "       <rect x = \"40\" y = \"40\" width = \"70\" height =\"20\" fill = \"green\" fill-opacity = \"0\"/>"
        "   </g>"
        "</svg>",
        "<svg viewBox = \"0 0 200 200\">"
        "   <g  fill = \"red\" >"
        "      <g fill-opacity = \"0.5\">"
        "         <g fill-rule = \"evenodd\">"
        "            <g>"
        "                <polygon points=\"20,20 50,120 100,10 40,80 50,80\" stroke = \"blue\"/>"
        "            </g>"
        "         </g>"
        "      </g>"
        "   </g>"
        "   <g fill-opacity = \"0.8\" >"
        "       <rect x = \"40\" y = \"40\" width = \"70\" height =\"20\" fill = \"none\"/>"
        "   </g>"
        "</svg>",
        "<svg viewBox = \"0 0 200 200\">"
        "   <g fill = \"none\" fill-opacity = \"0\">"
        "       <polygon points=\"20,20 50,120 100,10 40,80 50,80\" stroke = \"blue\" fill = \"red\" fill-opacity = \"0.5\" fill-rule = \"evenodd\"/>"
        "   </g>"
        "   <g fill-opacity = \"0\" >"
        "       <rect x = \"40\" y = \"40\" width = \"70\" height =\"20\" fill = \"green\"/>"
        "   </g>"
        "</svg>"
    };

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QVERIFY(renderer.isValid());
        images[i] = QImage(200, 200, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
        if (i != 0) {
            QCOMPARE(images[0], images[i]);
        }
    }
}
void tst_QSvgRenderer::testStopOffsetOpacity()
{
    static const char *svgs[] = {
        //reference
        "<svg  viewBox=\"0 0 64 64\">"
         "<radialGradient id=\"MyGradient1\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"50\" r=\"30\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"0.0\" style=\"stop-color:red\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:green\"  stop-opacity=\"1\"/>"
          "<stop offset=\"1\" style=\"stop-color:yellow\"  stop-opacity=\"1\"/>"
         "</radialGradient>"
         "<radialGradient id=\"MyGradient2\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"70\" r=\"70\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"0.0\" style=\"stop-color:blue\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:violet\"  stop-opacity=\"1\"/>"
          "<stop offset=\"1\" style=\"stop-color:orange\"  stop-opacity=\"1\"/>"
         "</radialGradient>"
         "<rect  x=\"5\" y=\"5\" width=\"55\" height=\"55\" fill=\"url(#MyGradient1)\" stroke=\"black\" />"
         "<rect  x=\"20\" y=\"20\" width=\"35\" height=\"35\" fill=\"url(#MyGradient2)\"/>"
        "</svg>",
        //Stop Offset
        "<svg  viewBox=\"0 0 64 64\">"
         "<radialGradient id=\"MyGradient1\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"50\" r=\"30\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"abc\" style=\"stop-color:red\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:green\"  stop-opacity=\"1\"/>"
          "<stop offset=\"1\" style=\"stop-color:yellow\"  stop-opacity=\"1\"/>"
         "</radialGradient>"
         "<radialGradient id=\"MyGradient2\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"70\" r=\"70\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"-3.bc\" style=\"stop-color:blue\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:violet\"  stop-opacity=\"1\"/>"
          "<stop offset=\"1\" style=\"stop-color:orange\"  stop-opacity=\"1\"/>"
         "</radialGradient>"
         "<rect  x=\"5\" y=\"5\" width=\"55\" height=\"55\" fill=\"url(#MyGradient1)\" stroke=\"black\" />"
         "<rect  x=\"20\" y=\"20\" width=\"35\" height=\"35\" fill=\"url(#MyGradient2)\"/>"
        "</svg>",
        //Stop Opacity
        "<svg  viewBox=\"0 0 64 64\">"
         "<radialGradient id=\"MyGradient1\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"50\" r=\"30\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"0.0\" style=\"stop-color:red\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:green\"  stop-opacity=\"x.45\"/>"
          "<stop offset=\"1\" style=\"stop-color:yellow\"  stop-opacity=\"-3.abc\"/>"
         "</radialGradient>"
         "<radialGradient id=\"MyGradient2\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"70\" r=\"70\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"0.0\" style=\"stop-color:blue\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:violet\"  stop-opacity=\"-0.xy\"/>"
          "<stop offset=\"1\" style=\"stop-color:orange\"  stop-opacity=\"z.5\"/>"
         "</radialGradient>"
         "<rect  x=\"5\" y=\"5\" width=\"55\" height=\"55\" fill=\"url(#MyGradient1)\" stroke=\"black\" />"
         "<rect  x=\"20\" y=\"20\" width=\"35\" height=\"35\" fill=\"url(#MyGradient2)\"/>"
        "</svg>",
        //Stop offset and Stop opacity
        "<svg  viewBox=\"0 0 64 64\">"
         "<radialGradient id=\"MyGradient1\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"50\" r=\"30\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"abc\" style=\"stop-color:red\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:green\"  stop-opacity=\"x.45\"/>"
          "<stop offset=\"1\" style=\"stop-color:yellow\"  stop-opacity=\"-3.abc\"/>"
         "</radialGradient>"
         "<radialGradient id=\"MyGradient2\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"70\" r=\"70\" fx=\"20\" fy=\"20\">"
          "<stop offset=\"-3.bc\" style=\"stop-color:blue\"  stop-opacity=\"0.3\"/>"
          "<stop offset=\"0.5\" style=\"stop-color:violet\"  stop-opacity=\"-0.xy\"/>"
          "<stop offset=\"1\" style=\"stop-color:orange\"  stop-opacity=\"z.5\"/>"
         "</radialGradient>"
         "<rect  x=\"5\" y=\"5\" width=\"55\" height=\"55\" fill=\"url(#MyGradient1)\" stroke=\"black\" />"
         "<rect  x=\"20\" y=\"20\" width=\"35\" height=\"35\" fill=\"url(#MyGradient2)\"/>"
        "</svg>"
    };

    QImage images[4];
    QPainter p;

    for (int i = 0; i < 4; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        QVERIFY(renderer.isValid());
        images[i] = QImage(64, 64, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
    }
    QCOMPARE(images[0], images[1]);
    QCOMPARE(images[0], images[2]);
    QCOMPARE(images[0], images[3]);
}

void tst_QSvgRenderer::testUseElement()
{
    static const char *svgs[] = {
        // 0 - Use referring to non group node (1)
        "<svg viewBox = \"0 0 200 200\">"
        " <polygon points=\"20,20 50,120 100,10 40,80 50,80\"/>"
        " <polygon points=\"20,80 50,180 100,70 40,140 50,140\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" stroke-width = \"3\"/>"
        "</svg>",
        // 1
        "<svg viewBox = \"0 0 200 200\">"
        " <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\"/>"
        " <use y = \"60\" xlink:href = \"#usedPolyline\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" stroke-width = \"3\"/>"
        "</svg>",
        // 2
        "<svg viewBox = \"0 0 200 200\">"
        " <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\"/>"
        " <g fill = \" red\" fill-opacity =\"0.2\">"
        "<use y = \"60\" xlink:href = \"#usedPolyline\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" stroke-width = \"3\"/>"
        "</g>"
        "</svg>",
        // 3
        "<svg viewBox = \"0 0 200 200\">"
        " <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\"/>"
        " <g stroke-width = \"3\" stroke = \"yellow\">"
        "  <use y = \"60\" xlink:href = \"#usedPolyline\" fill = \" red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\"/>"
        " </g>"
        "</svg>",
        // 4 - Use referring to non group node (2)
        "<svg viewBox = \"0 0 200 200\">"
        " <polygon points=\"20,20 50,120 100,10 40,80 50,80\" fill = \"green\" fill-rule = \"nonzero\" stroke = \"purple\" stroke-width = \"4\" stroke-dasharray = \"1,1,3,1\" stroke-offset = \"3\" stroke-miterlimit = \"6\" stroke-linecap = \"butt\" stroke-linejoin = \"round\"/>"
        " <polygon points=\"20,80 50,180 100,70 40,140 50,140\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" stroke-width = \"3\" stroke-dasharray = \"1,1,1,1\" stroke-offset = \"5\" stroke-miterlimit = \"3\" stroke-linecap = \"butt\" stroke-linejoin = \"square\"/>"
        "</svg>",
        // 5
        "<svg viewBox = \"0 0 200 200\">"
        " <g fill = \"green\" fill-rule = \"nonzero\" stroke = \"purple\" stroke-width = \"4\" stroke-dasharray = \"1,1,3,1\" stroke-offset = \"3\" stroke-miterlimit = \"6\" stroke-linecap = \"butt\" stroke-linejoin = \"round\">"
        "  <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\" />"
        " </g>"
        " <g stroke = \"blue\" stroke-width = \"3\" stroke-dasharray = \"1,1,1,1\" stroke-offset = \"5\" stroke-miterlimit = \"3\" stroke-linecap = \"butt\" stroke-linejoin = \"square\">"
        "  <use y = \"60\" xlink:href = \"#usedPolyline\"  fill-opacity = \"0.7\" fill= \"red\" stroke = \"blue\" fill-rule = \"evenodd\"/>"
        " </g>"
        "</svg>",
        // 6
        "<svg viewBox = \"0 0 200 200\">"
        " <g fill = \"green\" fill-rule = \"nonzero\" stroke = \"purple\" stroke-width = \"4\" stroke-dasharray = \"1,1,3,1\" stroke-offset = \"3\" stroke-miterlimit = \"6\" stroke-linecap = \"butt\" stroke-linejoin = \"round\">"
        "  <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\" />"
        " </g>"
        " <g stroke-width = \"3\" stroke-dasharray = \"1,1,1,1\" stroke-offset = \"5\" stroke-miterlimit = \"3\" stroke-linecap = \"butt\" stroke-linejoin = \"square\" >"
        "  <use y = \"60\" xlink:href = \"#usedPolyline\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" />"
        " </g>"
        "</svg>",
        // 7 - Use referring to group node
        "<svg viewBox = \"0 0 200 200\">"
        " <g>"
        "  <circle cx=\"0\" cy=\"0\" r=\"100\" fill = \"red\" fill-opacity = \"0.6\"/>"
        "  <rect x = \"10\" y = \"10\" width = \"30\" height = \"30\" fill = \"red\" fill-opacity = \"0.5\"/>"
        "  <circle fill=\"#a6ce39\" cx=\"0\" cy=\"0\" r=\"33\" fill-opacity = \"0.5\"/>"
        " </g>"
        "</svg>",
        // 8
        "<svg viewBox = \"0 0 200 200\">"
        " <defs>"
        "  <g id=\"usedG\">"
        "   <circle cx=\"0\" cy=\"0\" r=\"100\" fill-opacity = \"0.6\"/>"
        "   <rect x = \"10\" y = \"10\" width = \"30\" height = \"30\"/>"
        "   <circle fill=\"#a6ce39\" cx=\"0\" cy=\"0\" r=\"33\" />"
        "  </g>"
        " </defs>"
        " <use xlink:href =\"#usedG\" fill = \"red\" fill-opacity =\"0.5\"/>"
        "</svg>",
        // 9
        "<svg viewBox = \"0 0 200 200\">"
        " <defs>"
        "  <g fill = \"blue\" fill-opacity = \"0.3\">"
        "   <g id=\"usedG\">"
        "    <circle cx=\"0\" cy=\"0\" r=\"100\" fill-opacity = \"0.6\"/>"
        "    <rect x = \"10\" y = \"10\" width = \"30\" height = \"30\"/>"
        "    <circle fill=\"#a6ce39\" cx=\"0\" cy=\"0\" r=\"33\" />"
        "   </g>"
        "  </g>"
        " </defs>"
        " <g fill = \"red\" fill-opacity =\"0.5\">"
        "  <use xlink:href =\"#usedG\" />"
        " </g>"
        "</svg>",
        // 10 - Self referral, should be ignored
        "<svg><g id=\"0\"><use xlink:href=\"#0\" /></g></svg>",
        // 11
        "<svg width=\"200\" height=\"200\">"
        "  <rect width=\"100\" height=\"50\"/>"
        "</svg>",
        // 12
        "<svg width=\"200\" height=\"200\">"
        "  <g id=\"0\"><use xlink:href=\"#0\" /><rect width=\"100\" height=\"50\"/></g>"
        "</svg>",
        // 13
        "<svg width=\"200\" height=\"200\">"
        "  <g id=\"0\"><g><use xlink:href=\"#0\" /><rect width=\"100\" height=\"50\"/></g></g>"
        "</svg>",
        // 14 (undefined)
        "<svg width=\"200\" height=\"200\">"
        "  <rect width=\"100\" height=\"50\"/>"
        "  <use x=\"100\" y=\"100\" opacity=\"0.5\" xlink:href=\"#nosuch\" />"
        "</svg>",
        // 15 - Forward references
        "<svg viewBox = \"0 0 200 200\">"
        " <use y = \"60\" xlink:href = \"#usedPolyline\" fill= \"red\" stroke = \"blue\" fill-opacity = \"0.7\" fill-rule = \"evenodd\" stroke-width = \"3\"/>"
        " <polygon id = \"usedPolyline\" points=\"20,20 50,120 100,10 40,80 50,80\"/>"
        "</svg>",
        // 16
        "<svg viewBox = \"0 0 200 200\">"
        " <use xlink:href =\"#usedG\" fill = \"red\" fill-opacity =\"0.5\"/>"
        " <defs>"
        "  <g id=\"usedG\">"
        "   <circle cx=\"0\" cy=\"0\" r=\"100\" fill-opacity = \"0.6\"/>"
        "   <rect x = \"10\" y = \"10\" width = \"30\" height = \"30\"/>"
        "   <circle fill=\"#a6ce39\" cx=\"0\" cy=\"0\" r=\"33\" />"
        "  </g>"
        " </defs>"
        "</svg>",
        // 17 - Indirect self referral
        "<svg>"
        " <defs>"
        "   <g id=\"g0\">"
        "     <g id=\"g1\"><use href=\"#g2\"/></g>"
        "     <g id=\"g2\"><use href=\"#g1\"/></g>"
        "   </g>"
        " </defs>"
        " <use xlink:href=\"#g0\" fill=\"black\"/>"
        "</svg>"
    };

    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        images[i] = QImage(200, 200, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();

        if (i < 4 && i != 0) {
            QCOMPARE(images[0], images[i]);
        } else if (i > 4 && i < 7) {
            if (sizeof(qreal) != sizeof(float))
            {
                // These images use blending functions which due to numerical
                // issues on Windows CE and likes differ in very few pixels.
                // For this reason an exact comparison will fail.
                QCOMPARE(images[4], images[i]);
            }
        } else if (i > 7 && i < 10) {
            QCOMPARE(images[8], images[i]);
        } else if (i == 12 || i == 13 || i == 17) {
            QCOMPARE(images[10], images[i]);
        } else if (i > 11 && i < 15) {
            QCOMPARE(images[11], images[i]);
        } else if (i == 15) {
            QCOMPARE(images[0], images[i]);
        } else if (i == 16) {
            QCOMPARE(images[8], images[i]);
        }
    }
}

void tst_QSvgRenderer::smallFont()
{
    static const char *svgs[] = { "<svg width=\"50px\" height=\"50px\"><text x=\"10\" y=\"10\" font-size=\"0\">Hello world</text></svg>",
                                  "<svg width=\"50px\" height=\"50px\"><text x=\"10\" y=\"10\" font-size=\"0.5\">Hello world</text></svg>"
    };
    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        if (i == 0)
            QTest::ignoreMessage(QtWarningMsg, "QFont::setPointSizeF: Point size <= 0 (0.000000), must be greater than 0");
        QSvgRenderer renderer(data);
        images[i] = QImage(50, 50, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
    }
    QVERIFY(images[0] != images[1]);
}

void tst_QSvgRenderer::styleSheet()
{
    static const char *svgs[] = { "<svg><style type=\"text/css\">.cls {fill:#ff0000;}</style><rect class=\"cls\" x = \"10\" y = \"10\" width = \"30\" height = \"30\"/></svg>",
                                  "<svg><style>.cls {fill:#ff0000;}</style><rect class=\"cls\" x = \"10\" y = \"10\" width = \"30\" height = \"30\"/></svg>",
    };
    const int COUNT = sizeof(svgs) / sizeof(svgs[0]);
    QImage images[COUNT];
    QPainter p;

    for (int i = 0; i < COUNT; ++i) {
        QByteArray data(svgs[i]);
        QSvgRenderer renderer(data);
        images[i] = QImage(50, 50, QImage::Format_ARGB32_Premultiplied);
        images[i].fill(-1);
        p.begin(&images[i]);
        renderer.render(&p);
        p.end();
    }
    QCOMPARE(images[0], images[1]);
}

void tst_QSvgRenderer::duplicateStyleId()
{
    QByteArray svg = QByteArrayLiteral("<svg><linearGradient id=\"a\"/>"
                                       "<rect style=\"fill:url(#a)\"/>"
                                       "<linearGradient id=\"a\"/></svg>");
    QTest::ignoreMessage(QtWarningMsg, "Duplicate unique style id: \"a\"");
    QImage image(200, 200, QImage::Format_RGB32);
    QPainter painter(&image);
    QSvgRenderer renderer(svg);
    renderer.render(&painter);
}

void tst_QSvgRenderer::oss_fuzz_23731()
{
    // when configured with "-sanitize undefined", this resulted in:
    // "runtime error: division by zero"
    QSvgRenderer().load(QByteArray("<svg><path d=\"A4------\">"));
}

void tst_QSvgRenderer::oss_fuzz_24131()
{
    // when configured with "-sanitize undefined", this resulted in:
    // "runtime error: -nan is outside the range of representable values of type 'int'"
    // runtime error: signed integer overflow: -2147483648 + -2147483648 cannot be represented in type 'int'
    QImage image(377, 233, QImage::Format_RGB32);
    QPainter painter(&image);
    QSvgRenderer renderer(QByteArray("<svg><path d=\"M- 4 44044404444E-334-\"/></svg>"));
    renderer.render(&painter);
}

void tst_QSvgRenderer::oss_fuzz_24738()
{
    // when configured with "-sanitize undefined", this resulted in:
    // "runtime error: division by zero"
    QSvgRenderer().load(QByteArray("<svg><path d=\"a 2 1e-212.....\">"));
}

QByteArray image_data_url(QImage &image) {
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadWrite);
    image.save(&buffer, "PNG");
    buffer.close();
    QByteArray url("data:image/png;base64,");
    url.append(data.toBase64());
    return url;
}

void tst_QSvgRenderer::imageRendering() {
    QImage img(2, 2, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::green);
    img.setPixel(0, 0, qRgb(255, 0, 0));
    img.setPixel(1, 1, qRgb(255, 0, 0));
    QByteArray imgurl(image_data_url(img));
    QString svgtemplate(
        "<svg><g transform='scale(2, 2)'>"
            "<image image-rendering='%1' xlink:href='%2' width='2' height='2' />"
        "</g></svg>"
    );
    const char *cases[] = {"optimizeQuality", "optimizeSpeed"};
    for (auto ir: cases) {
        QString svg = svgtemplate.arg(QLatin1String(ir)).arg(QLatin1String(imgurl));
        QImage img1(4, 4, QImage::Format_ARGB32);
        QPainter p1;
        p1.begin(&img1);
        QSvgRenderer renderer(svg.toLatin1());
        Q_ASSERT(renderer.isValid());
        renderer.render(&p1);
        p1.end();

        QImage img2(4, 4, QImage::Format_ARGB32);
        QPainter p2(&img2);
        p2.scale(2, 2);
        if (QLatin1String(ir) == QLatin1String("optimizeSpeed"))
            p2.setRenderHint(QPainter::SmoothPixmapTransform, false);
        else if (QLatin1String(ir) == QLatin1String("optimizeQuality"))
            p2.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p2.drawImage(0, 0, img);
        p2.end();
        QCOMPARE(img1, img2);
    }
}

void tst_QSvgRenderer::illegalAnimateTransform_data()
{
    QTest::addColumn<QByteArray>("svg");

    QTest::newRow("case1") << QByteArray("<svg><animateTransform type=\"rotate\" begin=\"1\" dur=\"2\" values=\"8,0,5,0\">");
    QTest::newRow("case2") << QByteArray("<svg><animateTransform type=\"rotate\" begin=\"1\" dur=\"2\" values=\"1,2\">");
    QTest::newRow("case3") << QByteArray("<svg><animateTransform type=\"rotate\" begin=\"1\" dur=\"2\" from=\".. 5 2\" to=\"f\">");
    QTest::newRow("case4") << QByteArray("<svg><animateTransform type=\"scale\" begin=\"1\" dur=\"2\" by=\"--,..\">");
}

void tst_QSvgRenderer::illegalAnimateTransform()
{
    QFETCH(QByteArray, svg);
    QSvgRenderer renderer;
    QVERIFY(!renderer.load(svg)); // also shouldn't assert
}

QTEST_MAIN(tst_QSvgRenderer)
#include "tst_qsvgrenderer.moc"
