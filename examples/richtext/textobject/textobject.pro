HEADERS         = svgtextobject.h \
                  window.h
SOURCES         = main.cpp \
                  svgtextobject.cpp \
                  window.cpp

QT += svg

RESOURCES       = resources.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/richtext/textobject
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/richtext/textobject
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
filesToDeploy.files = files/*.svg
filesToDeploy.path = files
DEPLOYMENT += filesToDeploy
