CONFIG += testcase
TARGET = tst_qicon_svg

QT += widgets testlib
SOURCES += tst_qicon_svg.cpp
RESOURCES = tst_qicon_svg.qrc

wince* {
   QT += xml svg
   DEPLOYMENT_PLUGIN += qsvg
}
TESTDATA += icons/*
