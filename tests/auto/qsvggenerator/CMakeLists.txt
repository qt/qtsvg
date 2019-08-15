# Generated from qsvggenerator.pro.

#####################################################################
## tst_qsvggenerator Test:
#####################################################################

add_qt_test(tst_qsvggenerator
    GUI
    SOURCES
        tst_qsvggenerator.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Svg
        Qt::Widgets
        Qt::Xml
)

# Resources:
set(qmake_immediate_resource_files
    "referenceSvgs/fileName_output.svg"
    "referenceSvgs/radial_gradient.svg"
)

add_qt_resource(tst_qsvggenerator "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

