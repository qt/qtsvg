FORMS     = forms/window.ui
HEADERS   = displaywidget.h \
            window.h
RESOURCES = svggenerator.qrc
SOURCES   = displaywidget.cpp \
            main.cpp \
            window.cpp

QT += svg

INCLUDEPATH += $$PWD

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/svg/svggenerator
sources.files = $$SOURCES $$HEADERS $$RESOURCES forms doc resources svggenerator.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/svg/svggenerator
INSTALLS += target sources

QT+=widgets
