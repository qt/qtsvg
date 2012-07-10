HEADERS         = svgtextobject.h \
                  window.h
SOURCES         = main.cpp \
                  svgtextobject.cpp \
                  window.cpp

QT += svg

RESOURCES       = resources.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/richtext/textobject
sources.files = $$SOURCES $$HEADERS *.pro $$RESOURCES files
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/richtext/textobject
INSTALLS += target sources

filesToDeploy.files = files/*.svg
filesToDeploy.path = files
DEPLOYMENT += filesToDeploy
QT+=widgets
