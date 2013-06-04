TEMPLATE = subdirs
qtHaveModule(widgets):!contains(QT_CONFIG, opengles2): SUBDIRS += framebufferobject
