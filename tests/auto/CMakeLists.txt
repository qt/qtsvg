# Generated from auto.pro.


if(TARGET Qt::Widgets)
    add_subdirectory(qsvgdevice)
    add_subdirectory(qsvggenerator)
    add_subdirectory(qsvgrenderer)
    add_subdirectory(qsvgplugin)
    add_subdirectory(qicon_svg)
endif()

if(NOT cross_compile)
endif()
