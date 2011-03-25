TEMPLATE=subdirs
SUBDIRS=\
           qsvgdevice \
           qsvggenerator \
           qsvgrenderer \

!cross_compile:                             SUBDIRS += host.pro
