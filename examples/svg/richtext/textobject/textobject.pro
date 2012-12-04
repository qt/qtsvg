HEADERS         = svgtextobject.h \
                  window.h
SOURCES         = main.cpp \
                  svgtextobject.cpp \
                  window.cpp

QT += svg

RESOURCES       = resources.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/svg/richtext/textobject
INSTALLS += target

filesToDeploy.files = files/*.svg
filesToDeploy.path = files
DEPLOYMENT += filesToDeploy
QT+=widgets
