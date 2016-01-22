TEMPLATE = subdirs

load(qfeatures)
!contains(QT_DISABLED_FEATURES, imageformatplugin): SUBDIRS += imageformats
SUBDIRS += iconengines
