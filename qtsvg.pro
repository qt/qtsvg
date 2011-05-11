TEMPLATE = subdirs

module_qtsvg_src.subdir = src
module_qtsvg_src.target = module-qtsvg-src

module_qtsvg_examples.subdir = examples
module_qtsvg_examples.target = module-qtsvg-examples
module_qtsvg_examples.depends = module_qtsvg_src

module_qtsvg_demos.subdir = demos
module_qtsvg_demos.target = module-qtsvg-demos
module_qtsvg_demos.depends = module_qtsvg_src

module_qtsvg_tests.subdir = tests
module_qtsvg_tests.target = module-qtsvg-tests
module_qtsvg_tests.depends = module_qtsvg_src
module_qtsvg_tests.CONFIG = no_default_install
!contains(QT_BUILD_PARTS,tests):module_qtsvg_tests.CONFIG += no_default_target

SUBDIRS += module_qtsvg_src \
           module_qtsvg_examples \
           module_qtsvg_demos \
           module_qtsvg_tests \

