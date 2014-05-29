/* This file is part of the KDE project
   Copyright (C) 2013 - 2014 Yue Liu <yue.liu@mail.com>

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

#ifndef KOFILEDIALOG_H
#define KOFILEDIALOG_H

#include "kowidgets_export.h"

#include <KUrl>
#include <QFileDialog>
#include <QString>
#include <QUrl>
#include <QStringList>
#include <QList>

/**
 * Wrapper around QFileDialog providing native file dialogs
 * on KDE/Gnome/Windows/OSX/etc.
 */
class KOWIDGETS_EXPORT KoFileDialog : public QObject
{
    Q_OBJECT

public:
    enum DialogType {
        OpenFile,
        OpenFiles,
        OpenDirectory,
        OpenDirectories,
        ImportFile,
        ImportFiles,
        ImportDirectory,
        ImportDirectories,
        SaveFile,
        SaveFiles
    };

    /**
     * @brief constructor
     * @param parent The parent of the file dialog
     * @param dialogType usage of the file dialog
     * @param dialogName the name for the file dialog. This will be used to open
     * the filedialog in the last open location, instead the specified directory.
     *
     * @return The name of the entry user selected in the file dialog
     *
     */
    KoFileDialog(QWidget *parent,
                 KoFileDialog::DialogType type,
                 const QString dialogName);

    ~KoFileDialog();

    void setCaption(const QString &caption);

    /**
     * @brief setDefaultDir set the default directory to defaultDir
     *
     * @param defaultDir a path to a file or directory
     */
    void setDefaultDir(const QString &defaultDir);

    /**
     * @brief setOverrideDir override both the default dir and the saved dir found by dialogName
     * @param overrideDir a path to a file or directory
     */
    void setOverrideDir(const QString &overrideDir);

    /**
     * @brief setImageFilters sets the name filters for the file dialog to all
     * image formats Qt's QImageReader supports.
     */
    void setImageFilters();

    void setNameFilter(const QString &filter);
    void setNameFilters(const QStringList &filterList,
                        const QString &defaultFilter = QString());
    void setMimeTypeFilters(const QStringList &filterList,
                            const QString &defaultFilter = QString());
    void setHideNameFilterDetailsOption();

    QStringList urls();
    QString url();


    /**
     * @brief selectedNameFilter returns the name filter the user selected, either
     *    directory or by clicking on it.
     * @return
     */
    QString selectedNameFilter() const;

private slots:

    void filterSelected(const QString &filter);

private:
    enum FilterType {
        MimeFilter,
        NameFilter
    };

    void createFileDialog();

    const QString getUsedDir(const QString &dialogName);
    void saveUsedDir(const QString &fileName, const QString &dialogName);

    const QStringList getFilterStringList(const QStringList &mimeList,
                                      bool withAllSupportedEntry = false);

    const QString getFilterString(const QStringList &mimeList,
                                  bool withAllSupportedEntry = false);

    const QString getFilterString(const QString &defaultMime);

    class Private;
    Private * const d;
};

#endif /* KOFILEDIALOG_H */
