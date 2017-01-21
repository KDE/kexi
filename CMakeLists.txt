cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
find_package(ECM 1.8.0 REQUIRED NOMODULE)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
include(SetKexiCMakePolicies NO_POLICY_SCOPE)
include(SetKexiVersionInfo)

project(Kexi VERSION ${PROJECT_VERSION})

include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(ECMPoQmTools)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include(KexiMacros)
include(KexiAddIconsRccFile)

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

# get the definitions of products, features and product sets
include(KexiProducts.cmake)

set(PRODUCTSET_DEFAULT "ALL")

if(NOT PRODUCTSET)
    set(PRODUCTSET ${PRODUCTSET_DEFAULT} CACHE STRING "Set of products/features to build" FORCE)
endif()

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
find_package(KF5 ${REQUIRED_KF5_VERSION} REQUIRED COMPONENTS
        Archive 
        Codecs 
        Completion
        Config 
        ConfigWidgets 
        CoreAddons
        GuiAddons 
        I18n 
        IconThemes 
        ItemViews
        KIO 
        TextEditor 
        TextWidgets 
        WidgetsAddons 
        XmlGui 
)
find_package(KF5 ${REQUIRED_KF5_VERSION} QUIET OPTIONAL_COMPONENTS Crash)
macro_bool_to_01(KF5Crash_FOUND HAVE_KCRASH)
macro_log_feature(${KF5Crash_FOUND} "KCrash" "KDE's Crash Handler"
                  "https://api.kde.org/frameworks/kcrash/html" FALSE ""
                  "Optionally used to provide crash reporting on Linux")

set(REQUIRED_QT_VERSION 5.4.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets Xml Network PrintSupport Test)
find_package(Qt5 ${REQUIRED_QT_VERSION} COMPONENTS UiTools WebKit WebKitWidgets)

# use sane compile flags
add_definitions(
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_STRICT_ITERATORS
  -DQT_USE_FAST_CONCATENATION
  -DQT_USE_FAST_OPERATOR_PLUS
  -DQT_USE_QSTRINGBUILDER
)

# only with COMPILING_TESTS definition will all the FOO_TEST_EXPORT macros do something
# TODO: check if this can be moved to only those places which make use of it,
# to reduce global compiler definitions that would trigger a recompile of
# everything on a change (like adding/removing tests to/from the build)
macro_bool_to_01(BUILD_TESTING COMPILING_TESTS)

# overcome some platform incompatibilities
if(WIN32)
    find_package(KDEWin REQUIRED)
endif()

# set custom Kexi plugin installdir
set(KEXI_PLUGIN_INSTALL_DIR ${PLUGIN_INSTALL_DIR}/${KEXI_BASE_PATH})

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

set(KEXI_LIBS_MIN_VERSION 3.0.90)

##
## Test for KDb
##
option(KEXI_DEBUG_GUI "Debugging GUI for Kexi (requires KDB_DEBUG_GUI to be set too)" OFF)
if(KEXI_DEBUG_GUI)
    set(KDB_REQUIRED_COMPONENTS DEBUG_GUI)
endif()
find_package(KDb ${KEXI_LIBS_MIN_VERSION} REQUIRED COMPONENTS ${KDB_REQUIRED_COMPONENTS})
macro_log_feature(KDb_FOUND "KDb" "A database connectivity and creation framework"
                            "http://community.kde.org/KDb" FALSE "" "Required by Kexi for data handling")

##
## Test for KReport
##
find_package(KReport ${KEXI_LIBS_MIN_VERSION} REQUIRED)
if (KReport_FOUND)
    if(NOT KREPORT_SCRIPTING)
        message(FATAL_ERROR "Kexi requires KReport package with scripting support enabled (KREPORT_SCRIPTING)")
    endif()
endif()

##
## Test for KPropertyWidgets
##
find_package(KPropertyWidgets ${KEXI_LIBS_MIN_VERSION} REQUIRED COMPONENTS KF)
macro_log_feature(KPropertyWidgets_FOUND "KPropertyWidgets" "A property editing framework with editor widget"
                                  "http://community.kde.org/KProperty" FALSE "" "Required by Kexi")

include(CheckIfQtGuiCanBeExecuted)
include(CheckGlobalBreezeIcons)

 ###########################
############################
## Optional dependencies  ##
############################
###########################

##
## Test for marble
##
set(MARBLE_MIN_VERSION "0.19.2")
find_package(KexiMarble)
if(NOT MARBLE_FOUND)
    set(MARBLE_INCLUDE_DIR "")
else()
    set(HAVE_MARBLE TRUE)
endif()
macro_log_feature(MARBLE_FOUND "Marble" "KDE World Globe Widget library" "https://marble.kde.org/" FALSE "${MARBLE_MIN_VERSION}" "Required by Kexi form map widget")

##
## Test for Qt WebKitWidgets
##
#TODO switch to Qt WebEngine
macro_bool_to_01(Qt5WebKitWidgets_FOUND HAVE_QTWEBKITWIDGETS)
macro_log_feature(Qt5WebKitWidgets_FOUND "Qt WebkitWidgets" "QWidgets module for Webkit, the HTML engine." "http://qt.io" FALSE "" "Required by Kexi web form widget")

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
endif()

# non-app directories are moved here because they can depend on SHOULD_BUILD_{appname} variables set above
add_subdirectory(cmake)

if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()

macro_display_feature_log()

calligra_product_deps_report("product_deps")
calligra_log_should_build()
