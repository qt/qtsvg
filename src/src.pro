TEMPLATE = subdirs
CONFIG += ordered
qtHaveModule(gui): SUBDIRS += svg plugins
qtHaveModule(widgets): SUBDIRS += svgwidgets
