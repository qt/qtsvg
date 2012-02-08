TEMPLATE=subdirs
SUBDIRS=\
           qsvgdevice \
           qsvggenerator \
           qsvgrenderer \
           qicon_svg \

!cross_compile:                             SUBDIRS += host.pro
