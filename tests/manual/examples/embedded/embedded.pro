TEMPLATE = subdirs
qtHaveModule(widgets): SUBDIRS += desktopservices fluidlauncher

# Disable platforms without process support
!qtConfig(process): SUBDIRS -= fluidlauncher
