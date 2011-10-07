TARGET = tst_qsvgrenderer
CONFIG += testcase
QT += svg testlib widgets

SOURCES += tst_qsvgrenderer.cpp
RESOURCES += resources.qrc

wince*|symbian {
   addFiles.files = *.svg *.svgz
   addFiles.path = .

   DEPLOYMENT += addFiles
   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qsvg
   }
}

!symbian: {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
