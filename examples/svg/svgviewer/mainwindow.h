// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class SvgView;

QT_BEGIN_NAMESPACE
class QAction;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsRectItem;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    bool loadFile(const QString &path);

public slots:
    void openFile();
    void exportImage();
    void setRenderer(int renderMode);

private slots:
    void updateZoomLabel();

private:
    QAction *m_nativeAction;
    QAction *m_glAction;
    QAction *m_imageAction;
    QAction *m_antialiasingAction;
    QAction *m_backgroundAction;
    QAction *m_outlineAction;

    SvgView *m_view;
    QLabel *m_zoomLabel;

    QString m_currentPath;
};

#endif
