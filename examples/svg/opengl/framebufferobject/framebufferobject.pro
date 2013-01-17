QT += widgets opengl svg

HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += framebufferobject.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/svg/opengl/framebufferobject
INSTALLS += target
