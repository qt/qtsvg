// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QFile>
#include <QSvgRenderer>

class tst_QSvgRenderer : public QObject
{
    Q_OBJECT

public:
    tst_QSvgRenderer();
    virtual ~tst_QSvgRenderer();

public slots:
    void init();
    void cleanup();

private slots:
    void construct();
    void load();
};

tst_QSvgRenderer::tst_QSvgRenderer()
{
}

tst_QSvgRenderer::~tst_QSvgRenderer()
{
}

void tst_QSvgRenderer::init()
{
}

void tst_QSvgRenderer::cleanup()
{
}

void tst_QSvgRenderer::construct()
{
    QBENCHMARK {
        QSvgRenderer renderer;
    }
}

void tst_QSvgRenderer::load()
{
    QFile file(":/data/tiger.svg");
    if (!file.open(QFile::ReadOnly))
        QFAIL("Can not open tiger.svg");
    QByteArray data = file.readAll();
    QSvgRenderer renderer;

    QBENCHMARK {
        renderer.load(data);
    }
}

QTEST_MAIN(tst_QSvgRenderer)
#include "tst_qsvgrenderer.moc"
