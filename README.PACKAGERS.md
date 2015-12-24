# Packaging Information for Kexi

We recommend building several binary packages from the Kexi source.

Splitting Kexi into packages:
- gives users a better choice of which components they have installed
- allows users to install just the applications without unnecessary dependencies
- helps to reduce packaging conflicts for users with non-standard package selections.

# Table Of Contents

1. Components  
1.1. Required: KTextEditor - advanced text editor library  
1.2. Kexi migration drivers to package separately  
1.3. Other Kexi plugins to package separately  
1.4. Kexi development files  
1.5. Quick command-line tests of Kexi installation  
2. Debug information  

# 1. Components

## 1.1. Required: KTextEditor -- advanced text editor library

Kexi uses the Kate editor's plugin and an advanced text editor library to implement editing of
SQL statements and scripts. The files are "libKF5TextEditor.so" and "katepart.so" among others.
Depending package
can be named like "libktexteditor".

**NOTE:** Kexi does not depend on the entire Kate application, just on the editing component.

## 1.2. Kexi migration drivers to package separately

Kexi provides migration drivers for a number of data sources. We encourage to put each driver 
in a separate package, and that installation of these packages be optional. Each driver package 
may then depend on the corresponding 'native' libraries.

### kexi-mysql-driver

Description: Kexi MySQL driver  
Migration driver files: keximigrate_mysql.so  
Dependencies: libmysqlclient

### kexi-postgresql-driver

Description: Kexi PostgreSQL driver  
Migration driver files: keximigrate_pqxx.so  
Dependencies: libpq

### kexi-sybase-driver

Description: Kexi Sybase & MS SQL driver  
Migration driver files: keximigrate_sybase.so  
Dependencies: libsybdb (FreeTDS)

### kexi-xbase-driver

Description: Kexi XBase driver  
Migration driver files: keximigrate_xbase.so  
Dependencies: libxbase

### kexi-spreadsheet-import

Description: Spreadsheet-to-Kexi-table import plugin  
Migration driver files: keximigrate_spreadsheet.so  
Translation File: keximigrate_spreadsheet.mo  
Dependencies: sheets (Calligra Sheets)

Plugin .so files are typically installed to $PREFIX/lib{64}/plugins/kexi/
and shared files installed to $PREFIX/share/kexi/.

## 1.3. Other Kexi plugins to package separately

Kexi provides less a number of plugin types that are optional either because are less
frequently used or because have larger dependencies. We encourage to put each driver in a
separate package, and that installation of these packages be optional.

### kexi-web-form-widget

Description: Kexi web form widget  
Contents: kformdesigner_webbrowser.so  
Translation File: kformdesigner_webbrowser.mo  
Dependencies: libQt5WebKit5 (provides Qt5 WebKit)

### kexi-map-form-widget

Description: Kexi map form widget  
Contents: kformdesigner_mapbrowser.so  
Translation File: kformdesigner_mapbrowser.mo  
Dependencies: libmarble5 or marble (provides libmarblewidget-qt5)

## 1.4. Kexi development files

Kexi ships no public development files at the moment, so -devel packages are not needed.

## 1.5. Quick command-line tests of Kexi installation

If you don't want to click through Kexi interface but still want to make (almost) sure the
application is properly packaged, please install it and type the following from the command
line:

    kexi --create-opendb --drv sqlite3 --new form testdb

(ignore possible warning messages)

This will:
- create a new empty database file "testdb",
- open it,
- create a new empty form


## 2. Debug information

For alpha and beta packages, please build with debug output enabled, but for production
packages the -DCMAKE_CXX_FLAGS="-DKDE_NO_DEBUG_OUTPUT" is recommended. A significant performance
increase will be the result.
