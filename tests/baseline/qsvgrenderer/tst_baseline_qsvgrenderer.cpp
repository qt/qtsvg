// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qbaselinetest.h>

#include <qsvgrenderer.h>
#include <QPainter>

#include <QtCore/QCoreApplication>
#include <QtCore/QDirIterator>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtGui/QImage>

#include <algorithm>

class tst_QSvgRenderer : public QObject
{
    Q_OBJECT

public:
    tst_QSvgRenderer();

private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void testRendering_data();
    void testRendering();

private:
    void setupTestSuite(const QByteArray& filter = QByteArray());
    void runTest(const QStringList& extraArgs = QStringList());
    bool renderAndGrab(const QString& qmlFile, const QStringList& extraArgs, QImage *screenshot, QString *errMsg);
    quint16 checksumFileOrDir(const QString &path);

    QString testSuitePath;
    QScopedPointer<QSvgRenderer> m_renderer;
};


tst_QSvgRenderer::tst_QSvgRenderer()
{

}


void tst_QSvgRenderer::initTestCase()
{
    m_renderer.reset(new QSvgRenderer());

    QString dataDir = QFINDTESTDATA("../data/.");
    if (dataDir.isEmpty())
        dataDir = QStringLiteral("data");
    QFileInfo fi(dataDir);
    if (!fi.exists() || !fi.isDir() || !fi.isReadable())
        QSKIP("Test suite data directory missing or unreadable: " + fi.canonicalFilePath().toLatin1());
    testSuitePath = fi.canonicalFilePath();

    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);
}


void tst_QSvgRenderer::cleanup()
{

}


void tst_QSvgRenderer::testRendering_data()
{
    setupTestSuite();
}


void tst_QSvgRenderer::testRendering()
{
    runTest();
}


void tst_QSvgRenderer::setupTestSuite(const QByteArray& filter)
{
    QTest::addColumn<QString>("svgFile");
    int numItems = 0;

    QStringList ignoreItems;
    QFile ignoreFile(testSuitePath + "/Ignore");
    if (ignoreFile.open(QIODevice::ReadOnly)) {
        while (!ignoreFile.atEnd()) {
            QByteArray line = ignoreFile.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'))
                ignoreItems += line;
        }
    }

    QStringList itemFiles;
    QDirIterator it(testSuitePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fp = it.next();
        if (fp.endsWith(".svg")) {
            QString itemName = fp.mid(testSuitePath.length() + 1);
            if (!ignoreItems.contains(itemName) && (filter.isEmpty() || !itemName.startsWith(filter)))
                itemFiles.append(it.filePath());
        }
    }

    std::sort(itemFiles.begin(), itemFiles.end());
    for (const QString &filePath : std::as_const(itemFiles)) {
        QByteArray itemName = filePath.mid(testSuitePath.length() + 1).toLatin1();
        QBaselineTest::newRow(itemName, checksumFileOrDir(filePath)) << filePath;
        numItems++;
    }

    if (!numItems)
        QSKIP("No svg test files found in " + testSuitePath.toLatin1());
}


void tst_QSvgRenderer::runTest(const QStringList& extraArgs)
{
    Q_UNUSED(extraArgs)

    QFETCH(QString, svgFile);

    m_renderer->load(svgFile);
    QImage actual(m_renderer->viewBox().size(), QImage::Format_RGB32);
    actual.fill(QColor(255, 255, 255));
    QPainter actualPainter(&actual);
    m_renderer->render(&actualPainter);

    QBASELINE_TEST(actual);
}



quint16 tst_QSvgRenderer::checksumFileOrDir(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isReadable())
        return 0;
    if (fi.isFile()) {
        QFile f(path);
        f.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray contents = f.readAll();
        return qChecksum(contents);
    }
    if (fi.isDir()) {
        static const QStringList nameFilters = QStringList() << "*.svg";
        quint16 cs = 0;
        const auto entryList = QDir(fi.filePath()).entryList(nameFilters,
                                                             QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &item : entryList)
            cs ^= checksumFileOrDir(path + QLatin1Char('/') + item);
        return cs;
    }
    return 0;
}


#define main _realmain
QTEST_MAIN(tst_QSvgRenderer)
#undef main

int main(int argc, char *argv[])
{
    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_qsvgrenderer.moc"
