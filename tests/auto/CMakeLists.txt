# Generated from auto.pro.


if(TARGET Qt::Widgets)
    add_subdirectory(qsvgdevice)
    if(TARGET Qt::Xml) # special case
        add_subdirectory(qsvggenerator) # special case
    endif() # special case
    add_subdirectory(qsvgrenderer)
    add_subdirectory(qsvgplugin)
    add_subdirectory(qicon_svg)
endif()

if(NOT cross_compile)
endif()
