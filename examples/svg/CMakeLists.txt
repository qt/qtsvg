# Generated from svg.pro.

add_subdirectory(embedded)
add_subdirectory(richtext)
add_subdirectory(draganddrop)
if(TARGET Qt::Widgets)
    add_subdirectory(embeddedsvgviewer)
    add_subdirectory(svggenerator)
    add_subdirectory(svgviewer)
endif()
