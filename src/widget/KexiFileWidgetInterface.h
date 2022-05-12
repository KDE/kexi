/* This file is part of the KDE project
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFILEWIDGETINTERFACE_H
#define KEXIFILEWIDGETINTERFACE_H

#include "kexiextwidgets_export.h"
#include <KexiFileFilters.h>
#include <QWidget>

//! @brief An interface for file widget supporting opening/saving files known by Kexi
class KEXIEXTWIDGETS_EXPORT KexiFileWidgetInterface
{
public:
    virtual ~KexiFileWidgetInterface();

    /**
     * @brief Creates a file widget
     * @param startDirOrVariable A URL specifying the initial directory and/or filename,
     *                 or using the @c kfiledialog:/// syntax to specify a last used location.
     *                 Refer to the KFileWidget::KFileWidget() documentation
     *                 for the @c kfiledialog:/// URL syntax.
     * @param mode File widget's mode
     * @param fileName Optional file name that is added to the resulting URL.
     * @param parent File widget's parent widget
     *
     * Depending on settings one of two file widget implementations is used:
     * - if the KEXI_USE_KFILEWIDGET build option is on and KDE Plasma desktop is detected as the
     *   current desktop, KF5's KFileWidget-based widget is created,
     * - if the KEXI_USE_KFILEWIDGET build option is off or if non-KDE Plasma desktop is detected
     *   as the current desktop, a simple KexiFileRequester widget is created.
     *
     * In addition, if the KEXI_USE_KFILEWIDGET build option is on, defaults can be overridden by
     * "UseKFileWidget" boolean option in the "File Dialogs" group of the application's config file:
     * - if "UseKFileWidget" is @c true, KF5's KFileWidget-based widget is created,
     * - if "UseKFileWidget" is @c false a simple KexiFileRequester widget is created.
     *
     * To delete the override, delete the "UseKFileWidget" option in the aplication's config file.
     *
     * @return the new file widget.
     *
     * @todo Share this code with KReport and Kexi
     */
    Q_REQUIRED_RESULT static KexiFileWidgetInterface *createWidget(const QUrl &startDirOrVariable,
                                                 KexiFileFilters::Mode mode,
                                                 const QString &fileName,
                                                 QWidget *parent = nullptr);

    /**
     * @overload
     */
    Q_REQUIRED_RESULT static KexiFileWidgetInterface *createWidget(const QUrl &startDirOrVariable,
                                                 KexiFileFilters::Mode mode,
                                                 QWidget *parent = nullptr);

    /**
     * @brief returns this object casted to QWidget pointer
     */
    inline QWidget *widget() { return dynamic_cast<QWidget*>(this); }

    //! @return mode for filters used in this widget
    KexiFileFilters::Mode mode() const;

    //! Sets mode for filters to be used in this widget
    void setMode(KexiFileFilters::Mode mode);

    //! @return additional mime types
    QStringList additionalMimeTypes() const;

    //! Sets additional mime types, e.g. "text/x-csv"
    void setAdditionalMimeTypes(const QStringList &mimeTypes);

    //! @return excluded mime types
    QStringList excludedMimeTypes() const;

    //! Set excluded mime types
    void setExcludedMimeTypes(const QStringList &mimeTypes);

    /**
     * Returns the full path of the selected file in the local filesystem.
     * (Local files only)
     */
    virtual QString selectedFile() const = 0;

    /**
     * @brief Sets the file name to preselect to @p name
     *
     * This takes absolute URLs and relative file names.
     */
    virtual void setSelectedFile(const QString &name) = 0;

    /**
     * @brief Returns @c true if the current URL meets requied constraints, e.g. exists
     *
     * Shows appropriate message box if needed.
     */
    bool checkSelectedFile();

    /**
     * @brief Returns the full path of the highlighted file in the local filesystem
     *
     * (Local files only)
     */
    virtual QString highlightedFile() const = 0;

    //! Sets location text
    //! @todo
    //virtual void setLocationText(const QString& text) = 0;

    //! @return default extension
    QString defaultExtension() const;

    /**
     * @brief Sets default extension which will be added after accepting if user didn't provided one
     * This method is usable when there is more than one filter so there is no rule what extension
     * should be selected. By default first one is selected.
     */
    void setDefaultExtension(const QString& ext);

    /**
     * @return @c true if user should be asked to accept overwriting existing file.
     * @see setConfirmOverwrites
     */
    bool confirmOverwrites() const;

    /*! If true, user will be asked to accept overwriting existing file.
    This is true by default. */
    void setConfirmOverwrites(bool set);

    /**
     * Sets whether the line edit draws itself with a frame.
     */
    virtual void setWidgetFrame(bool set) = 0;

    /**
     * @returns the currently shown directory.
     * Reimplement it.
     */
    virtual QString currentDir() const;

    /**
     * @brief Connects "file hightlighted" signal to specific receiver
     *
     * Connects widget's "fileHighlighted(QString)" signal to @a receiver and @a slot. The signal
     * is emit when a file item is selected or highlighted.
     *
     * @note Highlighting happens mostly when user single clicks a file item and
     * double-click-to-select mode is enabled (see KexiUtils::activateItemsOnSingleClick()). Rather
     * depend on file delecting than file highlighting.
     *
     * @see connectFileSelectedSignal
     */
    void connectFileHighlightedSignal(QObject *receiver, const char *slot);

    /**
     * @brief Connects "file selected" signal to specific receiver
     *
     * Connects "fileSelected(QString)" signal of widget's returned by widget() to
     * @a receiver and @a slot.
     */
    void connectFileSelectedSignal(QObject *receiver, const char *slot);

protected:
    KexiFileWidgetInterface(const QUrl &startDirOrVariable, const QString &fileName);

    /**
     * @brief Updates filters in the widget based on current filter selection.
     */
    virtual void updateFilters() = 0;

    /**
     * @brief Applies filename entered in the location edit
     *
     * Matching file item is selected on the files list if possible.
     */
    virtual void applyEnteredFileName() = 0;

    virtual QStringList currentFilters() const = 0;

    QUrl startUrl() const;

    void addRecentDir(const QString &name);

    KexiFileFilters* filters();

    const KexiFileFilters* filters() const;

    void setFiltersUpdated(bool set);

    bool filtersUpdated() const;

    void done();

private:
    class Private;
    Private * const d;
};

#endif // KEXIFILEWIDGETINTERFACE_H
