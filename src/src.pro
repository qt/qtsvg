TEMPLATE = subdirs
CONFIG += ordered
!isEmpty(QT.gui.name): SUBDIRS += svg plugins
