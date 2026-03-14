// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/QTest>

#include "../../../src/plugins/imageformats/svg/qsvgiohandler.cpp"
#include <QFile>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QSize>
#include <QVariant>

QStringList logMessages;

static void messageHandler(QtMsgType pType, const QMessageLogContext& pContext, const QString& pMsg)
{
    Q_UNUSED(pType);
    Q_UNUSED(pContext);
    logMessages.append(pMsg);
}


class tst_QSvgPlugin : public QObject
{
Q_OBJECT

public:
    tst_QSvgPlugin();
    virtual ~tst_QSvgPlugin();

private slots:
    void checkSize_data();
    void checkSize();
    void checkImageInclude();
    void encodings_data();
    void encodings();
    void animationProperties();
    void animationFrameReading();
    void staticSvgNoAnimation();
};



tst_QSvgPlugin::tst_QSvgPlugin()
{
}

tst_QSvgPlugin::~tst_QSvgPlugin()
{
}

void tst_QSvgPlugin::checkSize_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("imageHeight");
    QTest::addColumn<int>("imageWidth");

    QTest::newRow("square")              << QFINDTESTDATA("square.svg")              <<  50 <<  50;
    QTest::newRow("square_size")         << QFINDTESTDATA("square_size.svg")         << 200 << 200;
    QTest::newRow("square_size_viewbox") << QFINDTESTDATA("square_size_viewbox.svg") << 200 << 200;
    QTest::newRow("square_viewbox")      << QFINDTESTDATA("square_viewbox.svg")      << 100 << 100;
    QTest::newRow("tall")                << QFINDTESTDATA("tall.svg")                <<  50 <<  25;
    QTest::newRow("tall_size")           << QFINDTESTDATA("tall_size.svg")           << 200 << 100;
    QTest::newRow("tall_size_viewbox")   << QFINDTESTDATA("tall_size_viewbox.svg")   << 200 << 100;
    QTest::newRow("tall_viewbox")        << QFINDTESTDATA("tall_viewbox.svg")        << 100 <<  50;
    QTest::newRow("wide")                << QFINDTESTDATA("wide.svg")                <<  25 <<  50;
    QTest::newRow("wide_size")           << QFINDTESTDATA("wide_size.svg")           << 100 << 200;
    QTest::newRow("wide_size_viewbox")   << QFINDTESTDATA("wide_size_viewbox.svg")   << 100 << 200;
    QTest::newRow("wide_viewbox")        << QFINDTESTDATA("wide_viewbox.svg")        <<  50 << 100;
    QTest::newRow("invalid_xml")         << QFINDTESTDATA("invalid_xml.svg")         <<  0 << 0;
    QTest::newRow("xml_not_svg")         << QFINDTESTDATA("xml_not_svg.svg")         <<  0 << 0;
    QTest::newRow("invalid_then_valid")  << QFINDTESTDATA("invalid_then_valid.svg")  <<  0 << 0;
}

void tst_QSvgPlugin::checkSize()
{
    QFETCH(QString, filename);
    QFETCH(int, imageHeight);
    QFETCH(int, imageWidth);

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler plugin;
    plugin.setDevice(&file);

    QImage image;
    plugin.read(&image);

    // Check that plugin survives double load
    QVariant sizeVariant = plugin.option(QImageIOHandler::Size);

    file.close();

    QCOMPARE(imageHeight, image.height());
    QCOMPARE(imageWidth, image.width());

    QSize size = qvariant_cast<QSize>(sizeVariant);
    if (size.isEmpty())
        size = QSize(0, 0); // don't distinguish between null and invalid QSize
    QCOMPARE(size.width(), imageWidth);
    QCOMPARE(size.height(), imageHeight);
}

void tst_QSvgPlugin::checkImageInclude()
{
    const QString filename(QFINDTESTDATA("imageInclude.svg"));
    const QString path = filename.left(filename.size() - strlen("imageInclude.svg"));

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler plugin;
    plugin.setDevice(&file);

    QImage image;
    qInstallMessageHandler(messageHandler);
    plugin.read(&image);
    qInstallMessageHandler(nullptr);

    file.close();

    QCOMPARE(logMessages.size(), 8);
    QCOMPARE(logMessages.at(0), QString("Could not create image from \"%1notExisting.svg\"").arg(path));
    QCOMPARE(logMessages.at(1), QString("Could not create image from \"%1./notExisting.svg\"").arg(path));
    QCOMPARE(logMessages.at(2), QString("Could not create image from \"%1../notExisting.svg\"").arg(path));
    QCOMPARE(logMessages.at(3), QString("Could not create image from \"%1notExisting.svg\"").arg(QDir::rootPath()));
    QCOMPARE(logMessages.at(4), QLatin1String("Could not create image from \":/notExisting.svg\""));
    QCOMPARE(logMessages.at(5), QLatin1String("Could not create image from \"qrc:///notExisting.svg\""));
    QCOMPARE(logMessages.at(6), QLatin1String("Could not create image from \"file:///notExisting.svg\""));
    QCOMPARE(logMessages.at(7), QLatin1String("Could not create image from \"http://qt.io/notExisting.svg\""));

    logMessages.clear();
}

void tst_QSvgPlugin::encodings_data()
{
    QTest::addColumn<QString>("filename");

    QTest::newRow("utf-8") << QFINDTESTDATA("simple_Utf8.svg");
    QTest::newRow("utf-8_z") << QFINDTESTDATA("simple_Utf8.svgz");
    QTest::newRow("utf-16LE") << QFINDTESTDATA("simple_Utf16LE.svg");
    QTest::newRow("utf-16BE") << QFINDTESTDATA("simple_Utf16BE.svg");
    QTest::newRow("utf-32LE") << QFINDTESTDATA("simple_Utf32LE.svg");
    QTest::newRow("utf-32BE") << QFINDTESTDATA("simple_Utf32BE.svg");
    QTest::newRow("utf-32BE_z") << QFINDTESTDATA("simple_Utf32BE.svg.gz");
}

void tst_QSvgPlugin::encodings()
{
    QFETCH(QString, filename);

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(QSvgIOHandler::canRead(&file));
    }

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSvgIOHandler plugin;
    plugin.setDevice(&file);
    QVERIFY(plugin.canRead());
    QImage img;
    QVERIFY(plugin.read(&img));
    QCOMPARE(img.size(), QSize(50, 50));
}

void tst_QSvgPlugin::animationProperties()
{
    QFile file(QFINDTESTDATA("animated.svg"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler handler;
    handler.setDevice(&file);

    QVERIFY(handler.supportsOption(QImageIOHandler::Animation));

    // Trigger load via size query
    handler.option(QImageIOHandler::Size);

    QCOMPARE(handler.option(QImageIOHandler::Animation).toBool(), true);
    QVERIFY(handler.imageCount() > 1);
    QVERIFY(handler.nextImageDelay() > 0);
    QCOMPARE(handler.loopCount(), 0);
    QCOMPARE(handler.currentImageNumber(), -1);
}

void tst_QSvgPlugin::animationFrameReading()
{
    QFile file(QFINDTESTDATA("animated.svg"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler handler;
    handler.setDevice(&file);

    // Read first frame
    QImage frame;
    QVERIFY(handler.canRead());
    QVERIFY(handler.read(&frame));
    QCOMPARE(frame.size(), QSize(100, 100));
    QCOMPARE(handler.currentImageNumber(), 1);

    // Read all remaining frames sequentially
    const int total = handler.imageCount();
    int framesRead = 1;
    while (handler.canRead() && handler.read(&frame))
        ++framesRead;
    QCOMPARE(framesRead, total);
    QCOMPARE(handler.currentImageNumber(), total);

    // jumpToImage should work
    QVERIFY(handler.jumpToImage(0));
    QCOMPARE(handler.currentImageNumber(), 0);
    QVERIFY(handler.read(&frame));
    QCOMPARE(frame.size(), QSize(100, 100));
    QCOMPARE(handler.currentImageNumber(), 1);

    // jumpToImage out of range should fail
    QVERIFY(!handler.jumpToImage(-1));
    QVERIFY(!handler.jumpToImage(total));

    // jumpToNextImage should work
    QVERIFY(handler.jumpToImage(0));
    QVERIFY(handler.jumpToNextImage());
    QCOMPARE(handler.currentImageNumber(), 1);

    // canRead should return false after exhausting all frames
    QVERIFY(handler.jumpToImage(total - 1));
    QVERIFY(handler.read(&frame));
    QVERIFY(!handler.canRead());
    QVERIFY(!handler.read(&frame));
}

void tst_QSvgPlugin::staticSvgNoAnimation()
{
    QFile file(QFINDTESTDATA("square.svg"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler handler;
    handler.setDevice(&file);

    QCOMPARE(handler.option(QImageIOHandler::Animation).toBool(), false);
    QCOMPARE(handler.imageCount(), 0);
    QCOMPARE(handler.nextImageDelay(), 0);
    QCOMPARE(handler.loopCount(), 0);
    QCOMPARE(handler.currentImageNumber(), 0);

    QImage image;
    QVERIFY(handler.read(&image));
    QCOMPARE(handler.currentImageNumber(), 0);
    QCOMPARE(image.size(), QSize(50, 50));

    // Second read should fail for static SVG
    QVERIFY(!handler.read(&image));

    // Jump operations should fail for static SVG
    QVERIFY(!handler.jumpToImage(0));
    QVERIFY(!handler.jumpToNextImage());
}

QTEST_MAIN(tst_QSvgPlugin)
#include "tst_qsvgplugin.moc"
