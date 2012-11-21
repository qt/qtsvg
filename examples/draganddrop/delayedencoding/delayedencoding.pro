QT          += svg

HEADERS     = mimedata.h \
              sourcewidget.h
RESOURCES   = delayedencoding.qrc
SOURCES     = main.cpp \
              mimedata.cpp \
              sourcewidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/draganddrop/delayedencoding
sources.files = $$SOURCES $$HEADERS *.pro $$RESOURCES images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/draganddrop/delayedencoding
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
QT+=widgets
