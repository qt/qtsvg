QT.svg.VERSION = 5.0.0
QT.svg.MAJOR_VERSION = 5
QT.svg.MINOR_VERSION = 0
QT.svg.PATCH_VERSION = 0

QT.svg.name = QtSvg
QT.svg.bins = $$QT_MODULE_BIN_BASE
QT.svg.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtSvg
QT.svg.private_includes = $$QT_MODULE_INCLUDE_BASE/QtSvg/$$QT.svg.VERSION
QT.svg.sources = $$QT_MODULE_BASE/src/svg
QT.svg.libs = $$QT_MODULE_LIB_BASE
QT.svg.plugins = $$QT_MODULE_PLUGIN_BASE
QT.svg.imports = $$QT_MODULE_IMPORT_BASE
QT.svg.depends = core gui
!contains(QT_CONFIG, no-widgets): QT.svg.depends += widgets
QT.svg.DEFINES = QT_SVG_LIB
