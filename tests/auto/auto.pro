TEMPLATE=subdirs
!contains(QT_CONFIG, no-widgets) {
    SUBDIRS = \
        qsvgdevice \
        qsvggenerator \
        qsvgrenderer \
        qicon_svg \
}
!cross_compile: SUBDIRS += host.pro
