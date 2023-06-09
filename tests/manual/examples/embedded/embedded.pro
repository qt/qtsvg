TEMPLATE = subdirs
qtHaveModule(widgets): SUBDIRS += desktopservices fluidlauncher weatherinfo

# Disable platforms without process support
!qtConfig(process): SUBDIRS -= fluidlauncher

# Requires QtNetwork
!qtHaveModule(network): SUBDIRS -= weatherinfo
