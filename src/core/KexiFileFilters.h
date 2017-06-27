/* This file is part of the KDE project
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIFILEFILTERS_H
#define KEXIFILEFILTERS_H

#include "kexicore_export.h"

class QMimeType;
class QString;
class QStringList;

//! A tool for handling file filters for Kexi
class KEXICORE_EXPORT KexiFileFilters
{
public:
    //! Filter mode
    enum Mode {
        Opening, //!< Opening opens existing database (or shortcut)
        CustomOpening, //!< Used for opening other files, like CSV
        SavingFileBasedDB, //!< Saving file-based database file
        CustomSavingFileBasedDB, //!< Used for saving other files, like CSV
        SavingServerBasedDB //!< Saving server-based (shortcut) file
    };

    KexiFileFilters();
    ~KexiFileFilters();

    Mode mode() const;

    void setMode(Mode mode);

    /*! Sets a default-filter, that is used when an empty filter is set.
     * By default, this is set to i18n("*|All Files")
     * @see defaultFilter
     */
    void setDefaultFilter(const QString &filter);

    /**
     * @return the default filter, used when an empty filter is set.
     * @see setDefaultFilter
     */
    QString defaultFilter() const;

    //! @return additional mime types
    QStringList additionalMimeTypes() const;

    //! Sets additional mime types, e.g. "text/x-csv"
    void setAdditionalMimeTypes(const QStringList &mimeTypes);

    //! @return excluded mime types
    QStringList excludedMimeTypes() const;

    //! Set excluded mime types
    void setExcludedMimeTypes(const QStringList &mimeTypes);

    //! @return glob patterns for all mime types for given mode
    //! Includes additional mime types and excludes miem types specified by excludedMimeTypes().
    QStringList allGlobPatterns() const;

    //! @return @c true if existing file is required
    //! This is true for Opening and CustomOpening modes.
    bool isExistingFileRequired() const;

    enum Format {
        QtFormat, //!< QFileDialog-compatible format, e.g. "Image files (*.png *.xpm *.jpg)", ";;" separators
        KDEFormat, //!< KDE-compatible format, e.g. "*.png *.xpm *.jpg|Image files (*.png *.xpm *.jpg)", "\\n" separators
        KUrlRequesterFormat //!< KUrlRequester-compatible format, e.g. "*.png *.xpm *.jpg|Image files", "\\n" separators
    };

    static QString separator(KexiFileFilters::Format format);

    //! @return filters based on supplied parameters in given format
    QString toString(Format format) const;

    //! @return list of filters based on supplied parameters in given format
    QStringList toList(Format format) const;

    //! @return filter string in given format
    static QString toString(const QMimeType &mime, Format format);

    //! @overload QString toString(const QMimeType &mime, Format format);
    static QString toString(const QString& mimeName, Format format);

    //! @overload QString toString(const QMimeType &mime, Format format);
    static QString toString(const QStringList &patterns, const QString &comment, Format format);

    //! Static version of QString KexiFileFilters::toString(Format format) const
    static QString toString(const QStringList& mimeNames, Format format);

    //! Static version of QStringList toList(Format format) const
    static QStringList toList(const QStringList& mimeNames, Format format);

private:
    class Private;
    Private * const d;
};

#endif
