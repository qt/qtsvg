# Generated from embedded.pro.

if(TARGET Qt::Widgets)
    add_subdirectory(desktopservices)
endif()
if(QT_FEATURE_process AND TARGET Qt::Widgets)
    add_subdirectory(fluidlauncher)
endif()
if(TARGET Qt::Network AND TARGET Qt::Widgets)
    add_subdirectory(weatherinfo)
endif()
