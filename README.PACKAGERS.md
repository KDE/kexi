# Packaging Information for Kexi

We recommend preparing several binary packages from the Kexi source.

Splitting Kexi into packages:
- gives users a better choice to install only those components they intend to use
- allows users to avoid unnecessary dependencies that otherwise would have to be installed
- helps to reduce packaging conflicts for users with non-standard package selections.

# Table Of Contents

1. Project description  
2. Runtime dependencies  
3. Migration drivers to package separately  
4. Other plugins to package separately  
5. Development files  
6. Quick command-line tests of Kexi installation  

# 1. Project description

Kexi is a visual database applications builder. It is licensed under the LGPL 2.0 or later.
To obtain complete description or metadata of the project please read the `src/data/org.kde.kexi.appdata.xml`
file. You can also find links to application screenshots suitable for use on web pages.

# 2. Runtime dependencies

In addition to dependencies indicated at build time such as program libraries,
this is a list of other runtime dependencies. Existence of each dependency is checked
at configure time before actual building process starts.

## Breeze icons theme

To maintain a consistent theme, Kexi project matches its application style with icons
selection. For this reason Breeze is the current default. Kexi does not use individual
(SVG) icon files, only the `breeze-icons.rcc` resource. Dark theme is not used.
To obtain the file, contents of the [breeze-icons.git](https://quickgit.kde.org/?p=breeze-icons.git) repository have to
be built with a `-DBINARY_ICONS_RESOURCE=ON` *CMake* option.

# 3. Migration plugins to package separately

Kexi provides migration plugins for a number of data sources or formats. We encourage to put each driver
in a separate package, and that installation of these packages be optional. Each plugin package 
may then depend on the corresponding 'native' libraries.

## kexi-mysql-migration

Description: Kexi plugin for importing MySQL databases  
Migration driver files: keximigrate_mysql.so  
Dependencies: libmysqlclient

## kexi-postgresql-migration

Description: Kexi plugin for importing PostgreSQL databases  
Migration driver files: keximigrate_postgresql.so  
Dependencies: libpq

## TODO, DISABLED for Kexi 3: kexi-sybase-migration

Description: Kexi plugin for importing Sybase and MS SQL databases  
Migration driver files: keximigrate_sybase.so  
Dependencies: libsybdb (FreeTDS)

## TODO, DISABLED for Kexi 3: kexi-xbase-driver

Description: Kexi plugin for importing dBase files  
Migration driver files: keximigrate_xbase.so  
Dependencies: libxbase

## TODO, DISABLED for Kexi 3: kexi-spreadsheet-import

Description: Kexi plugin for importing spreadsheet files  
Migration driver files: keximigrate_spreadsheet.so  
Translation File: keximigrate_spreadsheet.mo  
Dependencies: sheets (Calligra Sheets)

Plugin .so files are typically installed to $PREFIX/lib{64}/plugins/kexi/
and shared files installed to $PREFIX/share/kexi/.

# 4. Other plugins to package separately

Kexi provides less a number of plugin types that are optional either because are less
frequently used or because have larger dependencies. We encourage to put each driver in a
separate package, and that installation of these packages be optional.

## kexi-web-form-widget

Description: Kexi web form widget  
Contents: kformdesigner_webbrowser.so  
Translation File: kformdesigner_webbrowser.mo  
Dependencies: libQt5WebKit5 (provides Qt5 WebKit)

## TODO, DISABLED for Kexi 3: kexi-map-form-widget

Description: Kexi map form widget  
Contents: kformdesigner_mapbrowser.so  
Translation File: kformdesigner_mapbrowser.mo  
Dependencies: libmarble5 or marble (provides libmarblewidget-qt5)

# 5. Development files

Kexi ships no public development files at the moment, so -devel packages are not needed.

# 6. Quick command-line tests of Kexi installation

If you don't want to click through Kexi interface but still want to make (almost) sure the
application is properly packaged, please install it and type the following from the command
line:

    kexi --create-opendb --drv org.kde.kdb.sqlite --new form testdb

(ignore possible warning messages)

This will:
- create a new empty SQLite3-based file "testdb",
- open it,
- create a new empty form.
