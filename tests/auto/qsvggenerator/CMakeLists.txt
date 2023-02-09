# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsvggenerator Test:
#####################################################################

qt_internal_add_test(tst_qsvggenerator
    SOURCES
        tst_qsvggenerator.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Svg
        Qt::Widgets
        Qt::Xml
)

# Resources:
set(qmake_immediate_resource_files
    "referenceSvgs/fileName_output.svg"
    "referenceSvgs/radial_gradient.svg"
)

qt_internal_add_resource(tst_qsvggenerator "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

