TARGET = tst_qsvggenerator
CONFIG += testcase
QT += svg xml testlib widgets gui-private

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
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
