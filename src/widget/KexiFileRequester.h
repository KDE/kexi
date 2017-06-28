/* This file is part of the KDE project
   Copyright (C) 2016-2017 Jarosław Staniek <staniek@kde.org>

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

#include "KexiFileWidgetInterface.h"
#include <QWidget>

//! @brief A widget showing a line edit and a button, which invokes a file dialog
class KEXIEXTWIDGETS_EXPORT KexiFileRequester : public QWidget, public KexiFileWidgetInterface
{
    Q_OBJECT
public:
    explicit KexiFileRequester(const QUrl &fileOrVariable, KexiFileFilters::Mode mode,
                               QWidget *parent = nullptr);

    explicit KexiFileRequester(const QString &selectedFile, KexiFileFilters::Mode mode,
                               QWidget *parent = nullptr);

    ~KexiFileRequester() override;

    /**
     * Returns the full path of the selected file in the local filesystem.
     * (Local files only)
     */
    QString selectedFile() const override;

    /**
     * Returns the full path of the highlighted file in the local filesystem.
     * (Local files only)
     */
    QString highlightedFile() const override;

    /**
     * @return the currently shown directory.
     */
    QString currentDir() const override;

    //! Sets file mode
    //void setFileMode(KexiFileFilters::Mode mode);

    //! @return additional mime types
    //QStringList additionalMimeTypes() const;

    //! @return excluded mime types
    //QStringList excludedMimeTypes() const;

    //! @return the default filter, used when an empty filter is set
    //QString defaultFilter() const;

Q_SIGNALS:
    void fileHighlighted(const QString &name);
    void fileSelected(const QString &name);

public Q_SLOTS:
    //! Sets the url
    void setSelectedFile(const QString &name) override;

    /**
     * Sets whether the line edit draws itself with a frame.
     */
    void setWidgetFrame(bool set) override;

protected:
    /**
     * Updates filters in the widget based on current filter selection.
     */
    void updateFilters() override;

    void applyEnteredFileName() override;

    QStringList currentFilters() const override;

private:
    void init();

    Q_DISABLE_COPY(KexiFileRequester)
    class Private;
    Private * const d;
};

#endif // KEXIFILEREQUESTER_H
