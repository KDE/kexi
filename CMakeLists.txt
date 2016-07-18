project(Kexi)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

cmake_policy(SET CMP0002 OLD)
cmake_policy(SET CMP0017 NEW)
cmake_policy(SET CMP0022 OLD)

if (NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
    cmake_policy(SET CMP0026 OLD) 
    cmake_policy(SET CMP0046 OLD)
endif ()

if(NOT ${CMAKE_VERSION} VERSION_LESS 3.3.0)
    cmake_policy(SET CMP0059 OLD)
    cmake_policy(SET CMP0063 OLD)
endif()

# ensure out-of-source build
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" isBuildInSource)
if(isBuildInSource)
    message(FATAL_ERROR "Compiling Kexi inside the source directory is not possible. "
                        "Please refer to the build instruction https://community.kde.org/Kexi/Building")
endif()

 ######################
#######################
## Constants defines ##
#######################
######################

# define common versions of Kexi components used to generate KexiVersion.h
# update these version for every release:
set(KEXI_VERSION_STRING "3.0 Alpha")
set(KEXI_STABLE_VERSION_MAJOR 3) # 3 for 3.x, 4 for 4.x, etc.
set(KEXI_STABLE_VERSION_MINOR 0) # 0 for 3.0, 1 for 3.1, etc.
set(KEXI_VERSION_RELEASE 89)     # 89 for Alpha, increase for next test releases, set 0 for first Stable, etc.
set(KEXI_ALPHA 1) # uncomment only for Alpha
#set(KEXI_BETA 1) # uncomment only for Beta
#set(KEXI_RC 1) # uncomment only for RC
set(KEXI_YEAR 2016) # update every year

if(NOT DEFINED KEXI_ALPHA AND NOT DEFINED KEXI_BETA AND NOT DEFINED KEXI_RC)
    set(KEXI_STABLE 1) # do not edit
endif()

message(STATUS "Kexi version: ${KEXI_VERSION_STRING}")

# Define the generic version of the Kexi libraries here
# This makes it easy to advance it when the next Kexi release comes.
# 14 was the last GENERIC_KEXI_LIB_VERSION_MAJOR of the previous Kexi series
# (2.x) so we're starting with 15 in 3.x series.
if(KEXI_STABLE_VERSION_MAJOR EQUAL 3)
    math(EXPR GENERIC_KEXI_LIB_VERSION_MAJOR "${KEXI_STABLE_VERSION_MINOR} + 15")
else()
    # let's make sure we won't forget to update the "15"
    message(FATAL_ERROR "Reminder: please update offset == 15 used to compute GENERIC_KEXI_LIB_VERSION_MAJOR to something bigger")
endif()
set(GENERIC_KEXI_LIB_VERSION "${GENERIC_KEXI_LIB_VERSION_MAJOR}.0.0")
set(GENERIC_KEXI_LIB_SOVERSION "${GENERIC_KEXI_LIB_VERSION_MAJOR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      "${CMAKE_SOURCE_DIR}/cmake/modules")

message("Module path:" ${CMAKE_MODULE_PATH})

# fetch git revision for the current build

set(KEXI_GIT_SHA1_STRING "")
set(KEXI_GIT_BRANCH_STRING "")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
get_git_branch(GIT_BRANCH)

if(GIT_SHA1 AND GIT_BRANCH)
    string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1)
    set(KEXI_GIT_SHA1_STRING ${GIT_SHA1})
    set(KEXI_GIT_BRANCH_STRING ${GIT_BRANCH})
endif()

if(NOT DEFINED RELEASE_BUILD)
    # estimate mode by CMAKE_BUILD_TYPE content if not set on cmdline
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
    set(RELEASE_BUILD_TYPES "release" "relwithdebinfo" "minsizerel")
    list(FIND RELEASE_BUILD_TYPES "${CMAKE_BUILD_TYPE_TOLOWER}" INDEX)
    if (INDEX EQUAL -1)
        set(RELEASE_BUILD FALSE)
    else()
        set(RELEASE_BUILD TRUE)
    endif()
endif()
message(STATUS "Release build: ${RELEASE_BUILD}")

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
## Look for ECM, Qt, KF5 ##
###########################
##########################

find_package(ECM 1.7.0 REQUIRED NOMODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

# CMake macros
include(CheckFunctionExists)
include(GenerateExportHeader)
# ECM macros
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
# ECM KDE macros
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(MacroBoolTo01)
# own macros
include(MacroLogFeature)
include(MacroEnsureVersion)

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
set(KEXI_PLUGIN_INSTALL_DIR ${LIB_INSTALL_DIR}/kexi)

# TEMPORARY: for initial Qt5/KF5 build porting phase deprecation warnings are only annoying noise
# remove once code porting phase starts, perhaps first locally in product subdirs
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
    add_definitions(-Wno-deprecated -Wno-deprecated-declarations)
endif ()

 ###########################
############################
## Required dependencies  ##
############################
###########################

##
## Test for KDb
##
find_package(KDb 2.96.4 REQUIRED)
macro_log_feature(KDb_FOUND "KDb" "A database connectivity and creation framework"
                            "http://community.kde.org/KDb" FALSE "" "Required by Kexi and its plugins")

##
## Test for KReport
##
find_package(KReport 2.97.0 REQUIRED)
if (KReport_FOUND)
    if(NOT KREPORT_SCRIPTING)
        message(FATAL_ERROR "Kexi requires KReport package with scripting support enabled (KREPORT_SCRIPTING)")
    endif()
endif()

##
## Test for KPropertyWidgets
##
find_package(KPropertyWidgets 2.99.0 REQUIRED)
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
    ##
    ## Marble changed addMarbleWidget to setMarbleWidget in MarbleControlBox.h
    ## with commit ea177ca. This is for compatibility with older versions.
    ##
    find_file(MARBLECONTROLBOX_H MarbleControlBox.h PATHS ${MARBLE_INCLUDE_DIR} PATH_SUFFIXES marble)
    if(MARBLECONTROLBOX_H)
        file(READ ${MARBLECONTROLBOX_H} MARBLECONTROLBOX_H_CONTENT)
        string(REGEX MATCH "setMarbleWidget" SETMARBLEWIDGET "${MARBLECONTROLBOX_H_CONTENT}")
        if( SETMARBLEWIDGET )
            set(HAVE_SETMARBLEWIDGET TRUE)
        endif()
    else()
        message( WARNING "MarbleControlBox.h not found, could not properly set the SETMARBLEWIDGET define." )
    endif()
endif()
macro_log_feature(MARBLE_FOUND "Marble" "KDE World Globe Widget library" "https://marble.kde.org/" FALSE "${MARBLE_MIN_VERSION}" "Required by Kexi form map widget")

if(WIN32)
    set(LIB_INSTALL_DIR ${LIB_INSTALL_DIR}
                        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
                        LIBRARY ${INSTALL_TARGETS_DEFAULT_ARGS}
                        ARCHIVE ${INSTALL_TARGETS_DEFAULT_ARGS} )
    set(DATA_INSTALL_DIR "$ENV{APPDATA}")
    STRING(REGEX REPLACE "\\\\" "/" DATA_INSTALL_DIR ${DATA_INSTALL_DIR})
endif()
string(TOLOWER "${PROJECT_NAME}" _PROJECT_NAME_LOWER)
set(ICONS_INSTALL_DIR "${DATA_INSTALL_DIR}/${_PROJECT_NAME_LOWER}/icons")

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

include(MacroCalligraAddBenchmark)
include(MacroCalligraAddTest)

 ###################################################
####################################################
## Detect which products/features can be compiled ##
####################################################
###################################################

if (NOT WIN32)
    set(NOT_WIN TRUE)
endif()
if (NOT QT_MAC_USE_COCOA)
    set(NOT_COCOA TRUE)
endif()

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
    add_subdirectory(doc)
endif()

# non-app directories are moved here because they can depend on SHOULD_BUILD_{appname} variables set above
add_subdirectory(cmake)

macro_display_feature_log()

calligra_product_deps_report("product_deps")
calligra_log_should_build()
