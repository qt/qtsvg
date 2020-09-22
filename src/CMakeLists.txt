# Generated from src.pro.

if(TARGET Qt::Gui)
    add_subdirectory(svg)
    add_subdirectory(plugins)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(svgwidgets)
endif()
