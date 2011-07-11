TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .
VERSION = $$QT_VERSION

# Input
HEADERS += \
           demoapplication.h \
           fluidlauncher.h \
           pictureflow.h \
           slideshow.h

SOURCES += \
           demoapplication.cpp \
           fluidlauncher.cpp \
           main.cpp \
           pictureflow.cpp \
           slideshow.cpp

embedded{
    target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
    sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html config.xml screenshots slides
    sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
    INSTALLS += target sources
}

wince*{
    QT += svg

    BUILD_DIR = release
    if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
        BUILD_DIR = debug
    }

    executables.files = \
        $$QT_BUILD_TREE/qtsvg/examples/embedded/embeddedsvgviewer/$${BUILD_DIR}/embeddedsvgviewer.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/styleexample/$${BUILD_DIR}/styleexample.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/deform/$${BUILD_DIR}/deform.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/pathstroke/$${BUILD_DIR}/pathstroke.exe \
        $$QT_BUILD_TREE/qtbase/examples/graphicsview/elasticnodes/$${BUILD_DIR}/elasticnodes.exe \
        $$QT_BUILD_TREE/qtbase/examples/widgets/wiggly/$${BUILD_DIR}/wiggly.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/concentriccircles/$${BUILD_DIR}/concentriccircles.exe \
        $$QT_BUILD_TREE/qtbase/examples/draganddrop/$${BUILD_DIR}/fridgemagnets.exe

    executables.path = .

    files.files = $$PWD/screenshots $$PWD/slides $$PWD/../embeddedsvgviewer/shapes.svg
    files.path = .

    config.files = $$PWD/config_wince/config.xml
    config.path = .

    DEPLOYMENT += config files executables

    DEPLOYMENT_PLUGIN += qgif qjpeg qmng qsvg
}

symbian {
    load(data_caging_paths)
    CONFIG += qt_demo
    RSS_RULES = # Clear RSS_RULES, otherwise fluidlauncher will get put into QtDemos folder

    TARGET.UID3 = 0xA000A641

    defineReplace(regResourceDir) {
        symbian-abld|symbian-sbsv2 {
            return($${EPOCROOT}$$HW_ZDIR$$REG_RESOURCE_IMPORT_DIR/$$basename(1))
        } else {
            return($${QT_BUILD_TREE}/$$1)
        }
    }

    defineReplace(appResourceDir) {
        symbian-abld|symbian-sbsv2 {
            return($${EPOCROOT}$${HW_ZDIR}$${APP_RESOURCE_DIR}/$$basename(1))
        } else {
            return($${QT_BUILD_TREE}/$$1)
        }
    }

    executables.files = \
        $$QT_BUILD_TREE/qtbase/examples/embedded/styleexample/styleexample.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/deform/deform.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/pathstroke/pathstroke.exe \
        $$QT_BUILD_TREE/qtbase/examples/widgets/wiggly/wiggly.exe \
        $$QT_BUILD_TREE/qtbase/examples/network/qftp/qftp.exe \
        $$QT_BUILD_TREE/qtbase/examples/xml/saxbookmarks/saxbookmarks.exe \
        $$QT_BUILD_TREE/qtsvg/examples/embedded/desktopservices/desktopservices.exe \
        $$QT_BUILD_TREE/qtbase/examples/draganddrop/fridgemagnets/fridgemagnets.exe \
        $$QT_BUILD_TREE/qtbase/examples/widgets/softkeys/softkeys.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/raycasting/raycasting.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/flickable/flickable.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/digiflip/digiflip.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/lightmaps/lightmaps.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/flightinfo/flightinfo.exe

    executables.path = /sys/bin

    reg_resource.files = \
        $$regResourceDir(qtbase/examples/embedded/styleexample/styleexample_reg.rsc) \
        $$regResourceDir(qtbase/examples/painting/deform/deform_reg.rsc) \
        $$regResourceDir(qtbase/examples/pathstroke/pathstroke_reg.rsc) \
        $$regResourceDir(qtbase/examples/widgets/wiggly/wiggly_reg.rsc) \
        $$regResourceDir(qtbase/examples/network/qftp/qftp_reg.rsc)\
        $$regResourceDir(qtbase/examples/xml/saxbookmarks/saxbookmarks_reg.rsc) \
        $$regResourceDir(qtsvg/examples/embedded/desktopservices/desktopservices_reg.rsc) \
        $$regResourceDir(qtbase/examples/draganddrop/fridgemagnets/fridgemagnets_reg.rsc) \
        $$regResourceDir(qtbase/examples/widgets/softkeys/softkeys_reg.rsc) \
        $$regResourceDir(qtbase/examples/embedded/raycasting/raycasting_reg.rsc) \
        $$regResourceDir(qtbase/examples/embedded/flickable/flickable_reg.rsc) \
        $$regResourceDir(qtbase/examples/embedded/digiflip/digiflip_reg.rsc) \
        $$regResourceDir(qtbase/examples/embedded/lightmaps/lightmaps_reg.rsc) \
        $$regResourceDir(qtbase/examples/embedded/flightinfo/flightinfo_reg.rsc)

    contains(QT_CONFIG, phonon) {
        reg_resource.files += $$regResourceDir(qtphonon/examples/qmediaplayer/qmediaplayer_reg.rsc)
    }

    contains(QT_CONFIG, multimedia) {
        reg_resource.files += $$regResourceDir(qtmultimedia/examples/spectrum/app/spectrum_reg.rsc)
    }


    reg_resource.path = $$REG_RESOURCE_IMPORT_DIR

    resource.files = \
        $$appResourceDir(qtbase/examples/embedded/styledemo/styledemo.rsc) \
        $$appResourceDir(qtbase/examples/painting/deform/deform.rsc) \
        $$appResourceDir(qtbase/examples/painting/pathstroke/pathstroke.rsc) \
        $$appResourceDir(qtbase/examples/widgets/wiggly/wiggly.rsc) \
        $$appResourceDir(qtbase/examples/network/qftp/qftp.rsc)\
        $$appResourceDir(qtbase/examples/xml/saxbookmarks/saxbookmarks.rsc) \
        $$appResourceDir(qtsvg/examples/embedded/desktopservices/desktopservices.rsc) \
        $$appResourceDir(qtbase/examples/draganddrop/fridgemagnets/fridgemagnets.rsc) \
        $$appResourceDir(qtbase/examples/widgets/softkeys/softkeys.rsc) \
        $$appResourceDir(qtbase/examples/embedded/raycasting/raycasting.rsc) \
        $$appResourceDir(qtbase/examples/embedded/flickable/flickable.rsc) \
        $$appResourceDir(qtbase/examples/embedded/digiflip/digiflip.rsc) \
        $$appResourceDir(qtbase/examples/embedded/lightmaps/lightmaps.rsc) \
        $$appResourceDir(qtbase/examples/embedded/flightinfo/flightinfo.rsc)


    resource.path = $$APP_RESOURCE_DIR

    mifs.files = \
        $$appResourceDir(qtsvg/examples/embedded/fluidlauncher/fluidlauncher.mif) \
        $$appResourceDir(qtbase/examples/embedded/styledemo/styledemo.mif) \
        $$appResourceDir(qtbase/examples/painting/deform/deform.mif) \
        $$appResourceDir(qtbase/examples/painting/pathstroke/pathstroke.mif) \
        $$appResourceDir(qtbase/examples/widgets/wiggly/wiggly.mif) \
        $$appResourceDir(qtbase/examples/network/qftp/qftp.mif) \
        $$appResourceDir(qtbase/examples/xml/saxbookmarks/saxbookmarks.mif) \
        $$appResourceDir(qtsvg/examples/embedded/desktopservices/desktopservices.mif) \
        $$appResourceDir(qtbase/examples/draganddrop/fridgemagnets/fridgemagnets.mif) \
        $$appResourceDir(qtbase/examples/widgets/softkeys/softkeys.mif) \
        $$appResourceDir(qtbase/examples/embedded/raycasting/raycasting.mif) \
        $$appResourceDir(qtbase/examples/embedded/flickable/flickable.mif) \
        $$appResourceDir(qtbase/examples/embedded/digiflip/digiflip.mif) \
        $$appResourceDir(qtbase/examples/embedded/lightmaps/lightmaps.mif) \
        $$appResourceDir(qtbase/examples/embedded/flightinfo/flightinfo.mif)
    mifs.path = $$APP_RESOURCE_DIR

    contains(QT_CONFIG, svg) {
        executables.files += \
            $$QT_BUILD_TREE/qtsvg/examples/embedded/embeddedsvgviewer/embeddedsvgviewer.exe \
            $$QT_BUILD_TREE/qtsvg/examples/embedded/weatherinfo/weatherinfo.exe

        reg_resource.files += \
            $$regResourceDir(qtsvg/examples/embedded/embeddedsvgviewer/embeddedsvgviewer_reg.rsc) \
            $$regResourceDir(qtsvg/examples/embedded/weatherinfo/weatherinfo_reg.rsc)

        resource.files += \
            $$appResourceDir(qtsvg/examples/embedded/embeddedsvgviewer/embeddedsvgviewer.rsc) \
            $$appResourceDir(qtsvg/examples/embedded/weatherinfo/weatherinfo.rsc)

        mifs.files += \
            $$appResourceDir(qtsvg/examples/embedded/embeddedsvgviewer/embeddedsvgviewer.mif) \
            $$appResourceDir(qtsvg/examples/embedded/weatherinfo/weatherinfo.mif)

    }
    contains(QT_CONFIG, webkit) {
        executables.files += $$QT_BUILD_TREE/qtwebkit-examples-and-demos/examples/embedded/anomaly/anomaly.exe
        reg_resource.files += $$regResourceDir(qtwebkit-examples-and-demos/examples/embedded/anomaly/anomaly_reg.rsc)
        resource.files += $$appResourceDir(qtwebkit-examples-and-demos/examples/embedded/anomaly/anomaly.rsc)
        mifs.files += \
            $$appResourceDir(qtwebkit-examples-and-demos/examples/embedded/anomaly/anomaly.mif)

        isEmpty(QT_LIBINFIX) {
            # Since Fluidlauncher itself doesn't link webkit, we won't get dependency automatically
            executables.pkg_prerules += \
                "; Dependency to Qt Webkit" \
                "(0x200267C2), $${QT_MAJOR_VERSION}, $${QT_MINOR_VERSION}, $${QT_PATCH_VERSION},  {\"QtWebKit\"}"
        }
    }

    contains(QT_CONFIG, phonon) {
        executables.files += $$QT_BUILD_TREE/qtphonon/examples/qmediaplayer/qmediaplayer.exe
        resource.files += $$appResourceDir(qtphonon/examples/qmediaplayer/qmediaplayer.rsc)
        mifs.files += \
            $$appResourceDir(qtphonon/examples/qmediaplayer/qmediaplayer.mif)
    }

    contains(QT_CONFIG, multimedia) {
        executables.files += $$QT_BUILD_TREE/qtmultimedia/examples/spectrum/app/spectrum.exe
        executables.files += $$QT_BUILD_TREE/qtmultimedia/examples/spectrum/3rdparty/fftreal/fftreal.dll
        resource.files += $$appResourceDir(qtmultimedia/examples/spectrum/app/spectrum.rsc)
        mifs.files += \
            $$appResourceDir(qtmultimedia/examples/spectrum/app/spectrum.mif)
    }

    contains(QT_CONFIG, script) {
        executables.files += $$QT_BUILD_TREE/qtscript/examples/script/context2d/context2d.exe
        reg_resource.files += $$regResourceDir(qtscript/examples/script/context2d/context2d_reg.rsc)
        resource.files += $$appResourceDir(qtscript/examples/script/context2d/context2d.rsc)
        mifs.files += \
            $$appResourceDir(qtscript/examples/script/context2d/context2d.mif)
    }

    qmldemos = qmlcalculator qmlclocks qmldialcontrol qmleasing qmlflickr qmlphotoviewer qmltwitter
    contains(QT_CONFIG, declarative) {
        for(qmldemo, qmldemos) {
            executables.files += $$QT_BUILD_TREE/qtdeclarative/examples/embedded/$${qmldemo}/$${qmldemo}.exe
            reg_resource.files += $$regResourceDir(qtdeclarative/examples/embedded/$${qmldemo}/$${qmldemo}_reg.rsc)
            resource.files += $$appResourceDir(qtdeclarative/examples/embedded/$${qmldemo}/$${qmldemo}.rsc)
            mifs.files += $$appResourceDir(qtdeclarative/examples/embedded/$${qmldemo}/$${qmldemo}.mif)
        }
    }

    files.files = $$PWD/screenshots $$PWD/slides
    files.path = .

    config.files = $$PWD/config_s60/config.xml
    config.path = .

    viewerimages.files = $$PWD/../embeddedsvgviewer/shapes.svg
    viewerimages.path = /data/images/qt/demos/embeddedsvgviewer

    # demos/mediaplayer make also use of these files.
    desktopservices_music.files = \
        $$PWD/../desktopservices/data/*.mp3 \
        $$PWD/../desktopservices/data/*.wav
    desktopservices_music.path = /data/sounds

    desktopservices_images.files = $$PWD/../desktopservices/data/*.png
    desktopservices_images.path = /data/images

    saxbookmarks.files = $$QT_BUILD_TREE/examples/xml/saxbookmarks/frank.xbel
    saxbookmarks.files += $$QT_BUILD_TREE/examples/xml/saxbookmarks/jennifer.xbel
    saxbookmarks.path = /data/qt/saxbookmarks

    fluidbackup.files = backup_registration.xml
    fluidbackup.path = /private/$$replace(TARGET.UID3, 0x,)

    DEPLOYMENT += config files executables viewerimages saxbookmarks reg_resource resource \
        mifs desktopservices_music desktopservices_images fluidbackup

    contains(QT_CONFIG, declarative):for(qmldemo, qmldemos):include($$QT_BUILD_TREE/qtdeclarative/examples/embedded/$${qmldemo}/deployment.pri)

    DEPLOYMENT.installer_header = 0xA000D7CD

    TARGET.EPOCHEAPSIZE = 100000 20000000
}
