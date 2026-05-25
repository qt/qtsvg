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

using namespace Qt::StringLiterals;

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
};



tst_QSvgPlugin::tst_QSvgPlugin()
    = default;

tst_QSvgPlugin::~tst_QSvgPlugin()
    = default;

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

    QString msg = uR"(Could not create image from "%1%2notExisting.svg")"_s;
    QTest::ignoreMessage(QtWarningMsg, msg.arg(path, "").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg(path, "./").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg(path, "../").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg(QDir::rootPath(), "").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg(":/", "").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg("qrc:///", "").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg("file:///", "").toUtf8().data());
    QTest::ignoreMessage(QtWarningMsg, msg.arg("http://qt.io/", "").toUtf8().data());

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QSvgIOHandler plugin;
    plugin.setDevice(&file);

    QImage image;
    plugin.read(&image);

    file.close();
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

QTEST_MAIN(tst_QSvgPlugin)
#include "tst_qsvgplugin.moc"
