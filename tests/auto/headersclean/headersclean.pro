TARGET = tst_headersclean
CONFIG += testcase
SOURCES  += tst_headersclean.cpp
QT = core testlib

contains(QT_CONFIG,svg): QT += svg
