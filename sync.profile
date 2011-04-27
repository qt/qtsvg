%modules = ( # path to module name map
    "QtSvg" => "$basedir/src/svg",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
);
%mastercontent = (
    "core" => "#include <QtCore/QtCore>\n",
    "gui" => "#include <QtGui/QtGui>\n",
);
%modulepris = (
    "QtSvg" => "$basedir/modules/qt_svg.pri",
);
# Modules and programs, and their dependencies.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - "LATEST_REVISION", to always test against the latest revision.
#   - "LATEST_RELEASE", to always test against the latest public release.
#   - "THIS_REPOSITORY", to indicate that the module is in this repository.
%dependencies = (
    "QtSvg" => {
        "QtCore" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
        "QtGui" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
    },
);
