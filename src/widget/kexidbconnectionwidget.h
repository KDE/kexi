/* This file is part of the KDE project
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2014 Roman Shtemberko <shtemberko@gmail.com>

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

#ifndef KEXIDBCONNECTIONWIDGET_H
#define KEXIDBCONNECTIONWIDGET_H

#include "kexiextwidgets_export.h"
#include "ui_kexidbconnectionwidget.h"
#include "ui_kexidbconnectionwidgetdetails.h"

#include <kexiprojectdata.h>

#include <KGuiItem>

#include <QTabWidget>
#include <QDialog>

class QPushButton;
class KexiDBDriverComboBox;
class KexiDBConnectionTabWidget;

class KEXIEXTWIDGETS_EXPORT KexiDBConnectionWidget
            : public QWidget, protected Ui::KexiDBConnectionWidget
{
    Q_OBJECT

public:
    explicit KexiDBConnectionWidget(QWidget* parent = nullptr);
    virtual ~KexiDBConnectionWidget();

    /*! Sets project data \a data.
     \a shortcutFileName is only used to check if the file is writable
     (if no, "save changes" button will be disabled). */
    void setData(const KexiProjectData& data, const QString& shortcutFileName = QString());

    /*! Sets connection data \a data.
     \a shortcutFileName is only used to check if the file is writable
     (if no, "save changes" button will be disabled). */
    void setData(const KDbConnectionData& data,
                 const QString& shortcutFileName = QString());

    KexiProjectData data();

    //! \return a pointer to 'save changes' button. You can call hide() for this to hide it.
    QPushButton* saveChangesButton() const;

    //! \return a pointer to 'test connection' button. You can call hide() for this to hide it.
    QPushButton* testConnectionButton() const;

    KexiDBDriverComboBox *driversCombo() const;

    //! \return true if only connection data is managed by this widget
    bool connectionOnly() const;

Q_SIGNALS:
    //! emitted when data saving is needed
    void saveChanges();

    void loadDBList();

protected Q_SLOTS:
    void slotLocationRadioClicked();
    void slotCBToggled(bool on);
    void slotShowSavePasswordHelp();

protected:
    void setDataInternal(const KexiProjectData& data, bool connectionOnly,
                         const QString& shortcutFileName);

private:
    class Private;
    Private * const d;

    friend class KexiDBConnectionTabWidget;
    friend class KexiDBConnectionDialog;
};

class KEXIEXTWIDGETS_EXPORT KexiDBConnectionWidgetDetails
            : public QWidget, public Ui::KexiDBConnectionWidgetDetails
{
    Q_OBJECT
public:
    explicit KexiDBConnectionWidgetDetails(QWidget* parent = nullptr);
    ~KexiDBConnectionWidgetDetails();
};

class KEXIEXTWIDGETS_EXPORT KexiDBConnectionTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit KexiDBConnectionTabWidget(QWidget* parent = nullptr);
    virtual ~KexiDBConnectionTabWidget();

    /*! Sets connection data \a data.
     \a shortcutFileName is only used to check if the file is writable
     (if no, "save changes" button will be disabled). */
    void setData(const KexiProjectData& data, const QString& shortcutFileName = QString());

    void setData(const KDbConnectionData& data,
                 const QString& shortcutFileName = QString());

    KexiProjectData currentProjectData();

    //! \return true if 'save password' option is selected
    bool savePasswordOptionSelected() const;

Q_SIGNALS:
    //! emitted when test connection is needed
    void testConnection();

protected Q_SLOTS:
    void slotTestConnection();
    void slotSocketComboboxToggled(bool on);

protected:
    KexiDBConnectionWidget *mainWidget;
    KexiDBConnectionWidgetDetails* detailsWidget;

    friend class KexiDBConnectionDialog;
};


class KEXIEXTWIDGETS_EXPORT KexiDBConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    /*! Creates a new connection dialog for project data \a data.
     Not only connection data is visible but also database name and title.
     \a shortcutFileName is only used to check if the shortcut file is writable
     (if no, "save changes" button will be disabled).
     The shortcut file is in .KEXIS format.
     Connect to saveChanges() signal to react on saving changes.
     If \a shortcutFileName is empty, the button will be hidden.
     \a acceptButtonGuiItem allows to override default "Open" button's appearance. */
    KexiDBConnectionDialog(QWidget* parent, const KexiProjectData& data,
                           const QString& shortcutFileName = QString(),
                           const KGuiItem& acceptButtonGuiItem = KGuiItem());

    /*! Creates a new connection dialog for connection data \a data.
     Only connection data is visible: database name and title fields are hidden.
     \a shortcutFileName is only used to check if the shortcut file is writable
     (if no, "save changes" button will be disabled).
     The shortcut file is in .KEXIC format.
     See above constructor for more details. */
    KexiDBConnectionDialog(QWidget* parent, const KDbConnectionData& data,
                           const QString& shortcutFileName = QString(),
                           const KGuiItem& acceptButtonGuiItem = KGuiItem());

    ~KexiDBConnectionDialog();

    /*! \return project data displayed within the dialog.
     Information about database name and title can be empty if the dialog
     contain only a connection data (if second constructor was used). */
    KexiProjectData currentProjectData();

    //! \return true if 'save password' option is selected
    bool savePasswordOptionSelected() const;

    KexiDBConnectionWidget *mainWidget() const;
    KexiDBConnectionWidgetDetails* detailsWidget() const;

Q_SIGNALS:
    //! emitted when data saving is needed
    void saveChanges();

    //! emitted when test connection is needed
    void testConnection();

    void loadDBList();

private:
    void init(const KGuiItem& acceptButtonGuiItem);

    class Private;
    Private * const d;
};

#endif // KEXIDBCONNECTIONWIDGET_H
