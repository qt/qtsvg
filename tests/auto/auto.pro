TEMPLATE=subdirs
qtHaveModule(widgets) {
    SUBDIRS = \
        qsvgdevice \
        qsvggenerator \
        qsvgrenderer \
        qicon_svg \
        cmake
}
!cross_compile: SUBDIRS += host.pro
