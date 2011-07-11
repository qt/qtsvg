TEMPLATE = subdirs

module_qtsvg_src.subdir = src
module_qtsvg_src.target = module-qtsvg-src

module_qtsvg_examples.subdir = examples
module_qtsvg_examples.target = module-qtsvg-examples
module_qtsvg_examples.depends = module_qtsvg_src
!contains(QT_BUILD_PARTS,examples) {
    module_qtsvg_examples.CONFIG += no_default_install no_default_target
}

module_qtsvg_tests.subdir = tests
module_qtsvg_tests.target = module-qtsvg-tests
module_qtsvg_tests.depends = module_qtsvg_src
module_qtsvg_tests.CONFIG = no_default_install
!contains(QT_BUILD_PARTS,tests):module_qtsvg_tests.CONFIG += no_default_target

SUBDIRS += module_qtsvg_src \
           module_qtsvg_examples \
           module_qtsvg_tests \

