/* This file is part of the KDE project
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2012 Dimitrios T. Tanis <dimitrios.tanis@kdemail.net>

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

#ifndef KEXICONNECTIONSELECTORWIDGET_H
#define KEXICONNECTIONSELECTORWIDGET_H

#include <core/kexidbconnectionset.h>
#include <KexiFileFilters.h>
#include "kexiextwidgets_export.h"

#include <QTreeWidgetItem>

class QAbstractButton;
class KDbDriverMetaData;

//! An item for a single database connection
class KEXIEXTWIDGETS_EXPORT ConnectionDataLVItem : public QTreeWidgetItem
{
public:
    ConnectionDataLVItem(KDbConnectionData *data,
                         const KDbDriverMetaData &driverMetaData, QTreeWidget* list);
    ~ConnectionDataLVItem();

    void update(const KDbDriverMetaData& driverMetaData);

    using QTreeWidgetItem::data;
    KDbConnectionData *data() const {
        return m_data;
    }

protected:
    KDbConnectionData *m_data;
};

//! @short Widget that allows to select a database connection (file- or server-based)
/*! The widget allows to select database connection without choosing database itself.
*/
class KEXIEXTWIDGETS_EXPORT KexiConnectionSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    //! Defines connection type
    enum ConnectionType {
        FileBased = 1, //!< the widget displays file-based connection
        ServerBased = 2 //!< the widget displays server-based connection
    };

    //! Defines operation mode
    enum OperationMode {
        Opening = 1,
        Saving = 2
    };

    /*! Constructs a new widget which contains \a conn_set as connection set.
     \a conn_set can be altered, because Add/Edit/Remove buttons are available
     to users. \a startDirOrVariable can be provided to specify a start dir for file browser
     (it can also contain a configuration variable name with "kfiledialog:///" prefix
     as described in KRecentDirs documentation). */
    //! @todo KEXI3 add equivalent of kfiledialog:/// for startDirOrVariable
    KexiConnectionSelectorWidget(KexiDBConnectionSet *conn_set,
                                 const QUrl& startDirOrVariable,
                                 OperationMode mode,
                                 QWidget* parent = nullptr);

    virtual ~KexiConnectionSelectorWidget();

    /*! After accepting this dialog this method returns wherher user selected
     file- or server-based connection. */
    ConnectionType selectedConnectionType() const;

    /*! \return data of selected connection, if server-based connection was selected.
     Returns NULL if no selection has been made or file-based connection
     has been selected.
     @see selectedConnectionType()
    */
    KDbConnectionData* selectedConnectionData() const;

    /*! \return the name of database file, if file-based connection was selected.
     Returns empty string if no selection has been made or server-based connection
     has been selected.
    //! @note Call checkSelectedFile() first
     @see selectedConnectionType()
    */
    QString selectedFile() const;

    QTreeWidget* connectionsList() const;

    bool confirmOverwrites() const;

    bool hasSelectedConnection() const;

    /*! @return true if the current file URL meets requied constraints (i.e. the file exists)
     Shows appropriate message box if needed. */
    bool checkSelectedFile();

    //! @return highlighted file
    QString highlightedFile() const;

Q_SIGNALS:
    void connectionItemExecuted(ConnectionDataLVItem *item);
    void connectionItemHighlighted(ConnectionDataLVItem *item);
    void connectionSelected(bool hasSelected);
    void fileSelected(const QString &name);

public Q_SLOTS:
    void showSimpleConnection();
    void showAdvancedConnection();
    virtual void setFocus();

    /*! Hides helpers on the server based connection page
      (sometimes it's convenient not to have these):
    - "Select existing database server's connection..." (label at the top)
    - "Click "Back" button" (label at the bottom)
    - "Back" button itself */
    void hideHelpers();
    void hideConnectonIcon();
    void hideDescription();

    /*! Sets selected filename to @a name.
     Only works when selectedConnectionType()==FileBased. */
    void setSelectedFile(const QString &name);

    /*! If true, user will be asked to accept overwriting existing project.
     This is true by default. */
    void setConfirmOverwrites(bool set);

    void setFileMode(KexiFileFilters::Mode mode);

    void setAdditionalMimeTypes(const QStringList &mimeTypes);

    //! Sets excluded mime types
    void setExcludedMimeTypes(const QStringList& mimeTypes);

    void setFileWidgetFrameVisible(bool set);

protected Q_SLOTS:
    void slotConnectionItemExecuted(QTreeWidgetItem *item);
    void slotConnectionItemExecuted();
    void slotRemoteAddBtnClicked();
    void slotRemoteEditBtnClicked();
    void slotRemoteRemoveBtnClicked();
    void slotConnectionSelectionChanged();
    void slotPrjTypeSelected(QAbstractButton *btn);
    void slotFileConnectionSelected(const QString &name);
    void slotConnectionSelected();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

private:
    ConnectionDataLVItem* addConnectionData(KDbConnectionData* data);
    ConnectionDataLVItem* selectedConnectionDataItem() const;

    class Private;
    Private * const d;
};

#endif // KEXICONNECTIONSELECTORWIDGET_H
