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
target.path = $$[QT_INSTALL_EXAMPLES]/svg/svggenerator
INSTALLS += target

QT+=widgets
