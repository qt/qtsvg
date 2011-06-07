QT          += svg

HEADERS     = mimedata.h \
              sourcewidget.h
RESOURCES   = delayedencoding.qrc
SOURCES     = main.cpp \
              mimedata.cpp \
              sourcewidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/draganddrop/delayedencoding
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/itemviews/delayedencoding
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C614
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
symbian: warning(This example does not work on Symbian platform)
maemo5: warning(This example does not work on Maemo platform)
simulator: warning(This example does not work on Simulator platform)
