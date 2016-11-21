/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KEXIFILEREQUESTER_H
#define KEXIFILEREQUESTER_H

#include "kexiextwidgets_export.h"
#include <KexiFileFilters.h>
#include <QWidget>

class QUrl;

//! @brief A widget showing a line edit and a button, which invokes a file dialog
class KEXIEXTWIDGETS_EXPORT KexiFileRequester : public QWidget
{
    Q_OBJECT
public:
    explicit KexiFileRequester(const QUrl &url, QWidget *parent = nullptr);
    ~KexiFileRequester();

    //! @return the current url
    QUrl url() const;

    //! Sets file mode
    void setFileMode(KexiFileFilters::Mode mode);

    //! @return additional mime types
    QStringList additionalMimeTypes() const;

    //! @return excluded mime types
    QStringList excludedMimeTypes() const;

    //! @return the default filter, used when an empty filter is set
    QString defaultFilter() const;

public Q_SLOTS:
    //! Sets the url
    void setUrl(const QUrl &url);

    //! Set excluded mime types
    void setExcludedMimeTypes(const QStringList &mimeTypes);

    //! Sets additional mime types, e.g. "text/x-csv"
    void setAdditionalMimeTypes(const QStringList &mimeTypes);

    //! Sets a default-filter, that is used when an empty filter is set
    void setDefaultFilter(const QString &filter);

protected:
    Q_DISABLE_COPY(KexiFileRequester)
    class Private;
    Private * const d;
};

#endif // KEXIFILEREQUESTER_H
