QT += widgets opengl svg

HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += framebufferobject.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/opengl/framebufferobject
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.png *.svg
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/opengl/framebufferobject
INSTALLS += target sources
