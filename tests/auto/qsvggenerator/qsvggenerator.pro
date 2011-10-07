TARGET = tst_qsvggenerator
CONFIG += testcase
QT += svg xml testlib widgets

SOURCES += tst_qsvggenerator.cpp

wince*|symbian {
    addFiles.files = referenceSvgs
    addFiles.path = .
    DEPLOYMENT += addFiles
}

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else:!symbian {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
