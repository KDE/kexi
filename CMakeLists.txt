cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
find_package(ECM 1.8.0 REQUIRED NOMODULE)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
include(SetKexiCMakePolicies NO_POLICY_SCOPE)
include(SetKexiVersionInfo)

project(Kexi VERSION ${PROJECT_VERSION})

include(KexiAddTests)
kexi_add_tests(OFF)

# ECM
include(ECMAddAppIcon)
include(ECMAddTests)
include(ECMGenerateHeaders)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(ECMPoQmTools)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(KDECompilerSettings NO_POLICY_SCOPE)

# Own
include(KexiMacros)
include(KexiAddIconsRccFile)
include(KexiGenerateDependencyGraph)

ensure_out_of_source_build("Please refer to the build instruction https://community.kde.org/Kexi/Building")
get_git_revision_and_branch()
detect_release_build()

 #######################
########################
## Productset setting ##
########################
#######################

# For predefined productsets see the definitions in KexiProducts.cmake and
# in the files in the folder cmake/productsets.

# Finding out the products & features to build is done in 5 steps:
# 1. have the user define the products/features wanted, by giving a productset
# 2. estimate all additional required products/features
# 3. estimate which of the products/features can be build by external deps
# 4. find which products/features have been temporarily disabled due to problems
# 5. estimate which of the products/features can be build by internal deps

# get the special macros
include(CalligraProductSetMacros)

set(PRODUCTSET_DEFAULT "DESKTOP")

if(NOT PRODUCTSET OR PRODUCTSET STREQUAL "ALL")
    # Force the default set also when it is "ALL" because we can't build both desktop and mobile
    set(PRODUCTSET ${PRODUCTSET_DEFAULT} CACHE STRING "Set of products/features to build" FORCE)
endif()

# get the definitions of products, features and product sets
include(KexiProducts.cmake)

if (RELEASE_BUILD)
    set(KEXI_SHOULD_BUILD_STAGING FALSE)
else ()
    set(KEXI_SHOULD_BUILD_STAGING TRUE)
endif ()

# finally choose products/features to build
calligra_set_productset(${PRODUCTSET})

 ##########################
###########################
## Look for Qt, KF5 ##
###########################
##########################

set(REQUIRED_KF5_VERSION 5.16.0)
set(REQUIRED_KF5_COMPONENTS
    Archive
    Codecs
    Config
    ConfigWidgets
    CoreAddons
    GuiAddons
    I18n
    IconThemes
    ItemViews
    WidgetsAddons
    TextWidgets
    XmlGui
)
if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    list(APPEND REQUIRED_KF5_COMPONENTS
        Completion
        KIO
        TextEditor
        TextWidgets
    )
endif()
find_package(KF5 ${REQUIRED_KF5_VERSION} REQUIRED COMPONENTS ${REQUIRED_KF5_COMPONENTS})

find_package(KF5 ${REQUIRED_KF5_VERSION} COMPONENTS Crash)
macro_bool_to_01(KF5Crash_FOUND HAVE_KCRASH)
set_package_properties(KF5Crash PROPERTIES TYPE OPTIONAL
                       PURPOSE "Used to provide crash reporting on Linux")

set(REQUIRED_QT_VERSION 5.4.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets Xml Network PrintSupport Test)
find_package(Qt5 ${REQUIRED_QT_VERSION} COMPONENTS UiTools WebKit WebKitWidgets)

# use sane compile flags
add_definitions(
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_STRICT_ITERATORS
  -DQT_USE_QSTRINGBUILDER
)

# set custom KEXI plugin installdir
set(KEXI_PLUGIN_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}/${KEXI_BASE_PATH})

simple_option(BUILD_EXAMPLES "Build and install examples" ON)

macro_bool_to_01(BUILD_EXAMPLES COMPILING_EXAMPLES)

# set custom KEXI examples installdir
set(KEXI_EXAMPLES_INSTALL_DIR ${KDE_INSTALL_DATAROOTDIR}/${KEXI_BASE_PATH}/examples)

# TEMPORARY: for initial Qt5/KF5 build porting phase deprecation warnings are only annoying noise
# remove once code porting phase starts, perhaps first locally in product subdirs
#if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
#    add_definitions(-Wno-deprecated -Wno-deprecated-declarations)
#endif ()

 ###########################
############################
## Required dependencies  ##
############################
###########################

set(KEXI_FRAMEWORKS_MIN_VERSION ${PROJECT_VERSION})

##
## Test for KDb
##
simple_option(KEXI_DEBUG_GUI "Debugging GUI for Kexi (requires KDB_DEBUG_GUI to be set too)" OFF)
if(KEXI_DEBUG_GUI)
    set(KDB_REQUIRED_COMPONENTS DEBUG_GUI)
endif()
find_package(KDb ${KEXI_FRAMEWORKS_MIN_VERSION} REQUIRED COMPONENTS ${KDB_REQUIRED_COMPONENTS})
set_package_properties(KDb PROPERTIES TYPE REQUIRED
                       PURPOSE "Required by Kexi for data handling")

##
## Test for KReport
##
find_package(KReport ${KEXI_FRAMEWORKS_MIN_VERSION})
set_package_properties(KReport PROPERTIES TYPE REQUIRED
                       PURPOSE "Required by Kexi for report handling")
if (KReport_FOUND)
    if(NOT KREPORT_SCRIPTING)
        message(FATAL_ERROR "Kexi requires KReport package with scripting support enabled (KREPORT_SCRIPTING)")
    endif()
endif()

##
## Test for KPropertyWidgets
##
if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    find_package(KPropertyWidgets ${KEXI_FRAMEWORKS_MIN_VERSION} REQUIRED COMPONENTS KF)
    set_package_properties(KPropertyWidgets PROPERTIES TYPE REQUIRED
                           PURPOSE "Required by Kexi for handling properties")
else() # only KPropertyCore
    find_package(KPropertyCore ${KEXI_FRAMEWORKS_MIN_VERSION} REQUIRED COMPONENTS KF)
    set_package_properties(KPropertyCore PROPERTIES TYPE REQUIRED
                           PURPOSE "Required by Kexi for handling properties")
endif()
include(CheckIfQtGuiCanBeExecuted)

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
include(CheckGlobalBreezeIcons)
endif()
 ###########################
############################
## Optional dependencies  ##
############################
###########################

##
## Test for marble
##
set(MARBLE_MIN_VERSION "0.19.2")
find_package(Marble ${MARBLE_MIN_VERSION} CONFIG)
set_package_properties(Marble PROPERTIES
    DESCRIPTION "KDE World Globe Widget library"
    URL "https://marble.kde.org"
    TYPE RECOMMENDED
    PURPOSE "Required by Kexi form map widget"
)
if(Marble_FOUND)
    set(HAVE_MARBLE TRUE)
endif()
set_package_properties(GLIB2 PROPERTIES TYPE RECOMMENDED PURPOSE "${_REQUIRED_BY_MDB}")

##
## Test for Qt WebKitWidgets
##
#TODO switch to Qt WebEngine
macro_bool_to_01(Qt5WebKitWidgets_FOUND HAVE_QTWEBKITWIDGETS)
set_package_properties(Qt5WebKit PROPERTIES
                       DESCRIPTION "Webkit for Qt, the HTML engine."
                       URL "https://qt.io"
                       TYPE RECOMMENDED PURPOSE "Required by Kexi web form widget"
)
set_package_properties(Qt5WebKitWidgets PROPERTIES
                       DESCRIPTION "QWidgets module for Webkit, the HTML engine."
                       URL "https://qt.io"
                       TYPE RECOMMENDED PURPOSE "Required by Kexi web form widget"
)

 ##################
###################
## Helper macros ##
###################
##################

include(MacroKexiAddBenchmark)
include(MacroKexiAddTest)

#############################################
####      Temporarily broken products    ####
#############################################

# If a product does not build due to some temporary brokeness disable it here,
# by calling calligra_disable_product with the product id and the reason,
# e.g.:
# calligra_disable_product(APP_KEXI "isn't buildable at the moment")

#############################################
####     Calculate buildable products    ####
#############################################

calligra_drop_unbuildable_products()


#############################################
####     Setup product-depending vars    ####
#############################################

 ###################
####################
## Subdirectories ##
####################
###################

add_subdirectory(src)

if(SHOULD_BUILD_DOC)
    find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS DocTools)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()

# non-app directories are moved here because they can depend on SHOULD_BUILD_{appname} variables set above
add_subdirectory(cmake)

if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()

calligra_product_deps_report("product_deps")
calligra_log_should_build()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
