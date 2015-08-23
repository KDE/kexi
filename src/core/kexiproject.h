/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2012 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPROJECT_H
#define KEXIPROJECT_H

#include <QObject>

#include <KDbTristate>
#include <KDbObject>
#include <KDbResult>

#include "kexiprojectdata.h"
#include "kexipartitem.h"
#include "kexi.h"

/*! KexiProject implementation version.
 It is altered after every change:
 - major number is increased after KexiProject storage format change,
 - minor is increased after adding binary-incompatible change.
 Use KexiProject::versionMajor() and KexiProject::versionMinor() to get real project's version.
*/

#define KEXIPROJECT_VERSION_MAJOR 1
#define KEXIPROJECT_VERSION_MINOR 0

class QFileInfo;
class KDbConnection;
class KDbParser;
class KexiMainWindow;
class KexiWindow;

namespace KexiPart
{
class Part;
class Info;

struct MissingPart {
    QString name;
    QString className;
};
typedef QList<MissingPart> MissingPartsList;
}

/**
 * @brief A single project's controller and data structure.
 * It contains data connection, state, etc.
 */
class KEXICORE_EXPORT KexiProject : public QObject, public KDbObject, public KDbResultable
{
    Q_OBJECT

public:
    /*! Constructor 1. Creates a new object using \a pdata.
     \a handler can be provided to receive error messages during
     entire KexiProject object's lifetime. */
    explicit KexiProject(const KexiProjectData& pdata, KDbMessageHandler* handler = 0);

    /*! Constructor 2. Like above but sets predefined connections \a conn.
     The connection should be created using the same connection data
     as pdata->connectionData(). The connection will become owned by created KexiProject
     object, so do not destroy it. */
    KexiProject(const KexiProjectData& pdata, KDbMessageHandler* handler,
                KDbConnection* conn);

    ~KexiProject();

    /*! \return major version of KexiProject object.
     This information is retrieved from database when existing project is opened. */
    int versionMajor() const;

    /*! \return minor version of KexiProject object.
     @see versionMajor() */
    int versionMinor() const;

    /*! Opens existing project using project data.
     \return true on success */
    tristate open();

    /*! Like open().
     \return true on success.
     Additional \a incompatibleWithKexi, is set to false on failure when
     connection for the project was successfully started bu the project
     is probably not compatible with Kexi - no valid "kexidb_major_ver"
     value in "kexi__db" table.
     This is often the case for native server-based databases.
     If so, Kexi application can propose importing the database
     or linking it to parent project (the latter isn't yet implemented).
     For other types of errors the variable is set to true. */
    tristate open(bool *incompatibleWithKexi);

    /*! Creates new, empty project using project data.
     If \a forceOverwrite is true, existing database project is silently overwritten.
     KDbConnection is created (accessible then with KexiProject::dbConnection()).

     Since KexiProject inherits KDbObject, it is possible to get error message
     and other information on error.

     \return true on success, false on failure, and cancelled when database exists
     but \a forceOverwrite is false. */
    tristate create(bool forceOverwrite = false);

    /**
     * @return true if a we are connected to a database
     */
    bool isConnected();

    /**
     * @return internal identifier for part class @a partClass.
     * -1 is returned if the class is unknown.
     * While the part classes are unique strings like :org.kexi-project.table",
     * the identifiers are specific to the given physically stored project,
     * because sets of parts can differ from between various Kexi installations.
     */
    int idForClass(const QString &partClass) const;

    /**
     * @return part class for internal identifier.
     * Empty string is returned if the class is unknown.
     * @see idForClass()
     */
    QString classForId(int classId) const;

    /**
     * @return all items of a type \a i in this project
     */
    KexiPart::ItemDict* items(KexiPart::Info *i);

    /**
     * @return all items of a class \a partClass in this project
     * It is a convenience function.
     */
    KexiPart::ItemDict* itemsForClass(const QString &partClass);

    /**
     * Puts a list of items of a type \a i in this project into \a list.
     * You can then sort this list using ItemList::sort().
     */
    void getSortedItems(KexiPart::ItemList  *list, KexiPart::Info *i);

    /**
     * Puts a sorted list of items of a class \a partClass into \a list.
     * You can then sort this list using ItemList::sort().
     */
    void getSortedItemsForClass(KexiPart::ItemList *list, const QString &partClass);

    /**
     * @return item of class \a partClass and name \a name
     */
    KexiPart::Item* itemForClass(const QString &partClass, const QString &name);

    /**
     * @return item of type \a i and name \a name
     */
    KexiPart::Item* item(KexiPart::Info *i, const QString &name);

    /**
     * @return item for \a identifier
     */
    KexiPart::Item* item(int identifier);

    /**
     * @return the database connection associated with this project
     */
    KDbConnection *dbConnection() const;

    /**
     * @return the project's data
     */
    KexiProjectData *data() const;

    /*! Opens object pointed by \a item in a view \a viewMode.
     \a staticObjectArgs can be passed for static object
     (only works when part for this item is of type KexiPart::StaticPart).
     The new widget will be a child of \a parent. */
    KexiWindow* openObject(QWidget* parent, KexiPart::Item *item,
                           Kexi::ViewMode viewMode = Kexi::DataViewMode,
                           QMap<QString, QVariant>* staticObjectArgs = 0);

    //! For convenience
    KexiWindow* openObject(QWidget* parent, const QString &partClass,
                           const QString& name, Kexi::ViewMode viewMode = Kexi::DataViewMode);

    /*! Remove a part instance pointed by \a item.
     \return true on success. */
    bool removeObject(KexiPart::Item* item);

    /*! Renames a part instance pointed by \a item to a new name \a newName.
     \return true on success. */
    bool renameObject(KexiPart::Item* item, const QString& newName);

    /*! Renames a part instance pointed by \a item to a new name \a newName.
     \return true on success. */
    bool setObjectCaption(KexiPart::Item* item, const QString& newCaption);

    /*! Creates part item for given part \a info.
     Newly item will not be saved to the backend but stored in memory only
     (owned by project), and marked as "neverSaved" (see KexiPart::Item::neverSaved()).
     The item will have assigned a new unique caption like e.g. "Table15",
     and unique name like "table15", but no specific identifier
     (because id will be assigned on creation at the backend side).

     If \a suggestedCaption is not empty, it will be set as a caption
     (with number suffix, to avoid duplicated, e.g. "employees7"
     for "employees" sugested name). Name will be then built based
     on this caption using KDb::stringToIdentifier().

     This method is used before creating new object.
     \return newly created part item or NULL on any error. */
    KexiPart::Item* createPartItem(KexiPart::Info *info,
                                   const QString& suggestedCaption = QString());

    //! Added for convenience.
    KexiPart::Item* createPartItem(KexiPart::Part *part,
                                   const QString& suggestedCaption = QString());

    /*! Adds item \a item after it is successfully stored as an instance of part
     pointed by \a info. Also clears 'neverSaved' flag if \a item.
     Used by KexiWindow::storeNewData().
     @internal */
    void addStoredItem(KexiPart::Info *info, KexiPart::Item *item);

    /*! removes \a item from internal dictionaries. The item is destroyed
     after successful removal.
     Used to delete an unstored part item previously created with createPartItem(). */
    void deleteUnstoredItem(KexiPart::Item *item);

    /**
     * @returns parts metioned in the project meta tables but not available locally
     */
    KexiPart::MissingPartsList missingParts() const;

    KDbParser* sqlParser();

    /*! Shows dialog for creating new blank project,
     ans creates one. Dialog is not shown if option for automatic creation
     is checked or Kexi::startupHandler().projectData() was provided from command line.
     \a cancelled is set to true if creation has been cancelled (e.g. user answered
     no when asked for database overwriting, etc.
     \return true if database was created, false on error or when cancel was pressed */
    static KexiProject* createBlankProject(bool *cancelled, const KexiProjectData& data,
                                           KDbMessageHandler* handler = 0);

    /*! Drops project described by \a data. \return true on success.
     Use with care: Any KexiProject objects allocated for this project will become invalid! */
    static tristate dropProject(const KexiProjectData& data,
                                KDbMessageHandler* handler, bool dontAsk = false);

    //! Helper method to ask user "Could not  open file for reading and writing. Do you want to
    //! open the file as read only?". @return true if user agrees, false if user cancels opening.
    static bool askForOpeningNonWritableFileAsReadOnly(QWidget *parent, const QFileInfo &finfo);

    /*! Generates ID for private "document" like Relations window.
     Private IDs are negative numbers (while ID regular part instance's IDs are >0)
     Private means that the object is not stored as-is in the project but is somewhat
     generated and in most cases there is at most one unique instance document of such type (part).
     To generate this ID, just app-wide internal counter is used. */
    virtual int generatePrivateID();

    //! Closes connection. @return true on success.
    bool closeConnection();

    /*! Loads current user's data block, referenced by \a objectID and \a dataID
     and puts it to \a dataString.
     \return true on success, false on failure and cancelled when there is no such data block
     \sa storeUserDataBlock() removeUserDataBlock() copyUserDataBlock() KDbConnection::loadDataBlock(). */
    tristate loadUserDataBlock(int objectID, const QString& dataID, QString *dataString);

    /*! Stores current user's data block \a dataString, referenced by \a objectID and \a dataID.
     The block will be stored in "kexi__userdata" table
     If there is already such record in the table, it's simply overwritten.
     \return true on success
     \sa loadUserDataBlock() removeUserDataBlock() copyUserDataBlock() KDbConnection::storeDataBlock(). */
    bool storeUserDataBlock(int objectID, const QString& dataID, const QString &dataString);

    /*! Copies urrent user's data blocks referenced by \a sourceObjectID and pointed
     by optional \a dataID.
     \return true on success. Does not fail if blocks do not exist.
     Prior to copying, existing user data blocks are removed even if there is nothing to copy.
     Copied data blocks will have \a destObjectID object identifier assigned.
     Note that if \a dataID is not specified, all user data blocks found for the \a sourceObjectID
     will be copied.
     \sa loadUserDataBlock() storeUserDataBlock() removeUserDataBlock() KDbConnection::copyDataBlock(). */
    bool copyUserDataBlock(int sourceObjectID, int destObjectID, const QString &dataID = QString());

    /*! Removes current user's data block referenced by \a objectID and \a dataID.
     \return true on success. Does not fail if the block does not exist.
     Note that if \a dataID is not specified, all data blocks for this user and object will be removed.
     \sa loadUserDataBlock() storeUserDataBlock() copyUserDataBlock() KDbConnection::removeDataBlock(). */
    bool removeUserDataBlock(int objectID, const QString& dataID = QString());

protected:
    /*! Creates connection using project data.
     The connection will be readonly if data()->isReadOnly().
     \return true on success, otherwise false and appropriate error is set. */
    bool createConnection();

    bool initProject();

    //! Used in open() and open(bool*).
    tristate openInternal(bool *incompatibleWithKexi);

    /*! Kexi itself can define a number of internal database objects (mostly data structures),
     usually tables for it's own purposes.
     Even while at KexiDB library level, such "system" tables, like "kexi__objects", "kexi__objectdata"
     are created automatically on database project creation, this is not enough: there are objects
     needed specifically for Kexi but not for other applications utilizing KexiDB library.
     Example table created here for now is "kexi__blobs".

     This method is called on create() and open(): creates necessary objects
     if they are not yet existing. This especially allows to create to create these objects
     (on open) within a project made with previous Kexi version not supporting
     all currently defined structurtes. We're trying to be here as much backward compatible as possible.
     For this purpose, here's the complete list of currently created objects:
     - "kexi__blobs" - a table containing BLOBs data that can be accessed globally at Kexi projects
       from within any database-aware view (table views, forms, reports, etc.)

     @param insideTransaction Embed entire creation process inside a transaction

     \return true on successful object's creation. Objects are created only once, they are not overwritten.
    */
    bool createInternalStructures(bool insideTransaction);

    /*! \return Kexi part for \a item. */
    KexiPart::Part *findPartFor(const KexiPart::Item& item);

Q_SIGNALS:
    /** signal emitted on error */
    void error(const QString &title, KDbObject *obj);

    /** signal emitted on error (not KexiDB-related) */
    void error(const QString &msg, const QString &desc);

    /** New \a item has been stored. */
    void newItemStored(KexiPart::Item *item);

    /** instance pointed by \a item is removed */
    void itemRemoved(const KexiPart::Item &item);

    /** instance pointed by \a item is renamed */
    void itemRenamed(const KexiPart::Item &item, const QString& oldName);

    /** caption for instance pointed by \a item is changed */
    void itemCaptionChanged(const KexiPart::Item &item, const QString& oldCaption);

protected:
    bool createIdForPart(const KexiPart::Info& info);

private:
    /*! Checks whether the project's connection is read-only.
     If so, error message is set and false is returned. */
    bool checkWritable();

    /*! Retrieves basic information (class, id, name, caption)
     about all items of all types for this project.
     @return true on success. */
    bool retrieveItems();

    /**
     * Checks project's kexi__part table.
     * @a singlePartClass can be provided to only check specified part.
     * Internal identifiers of part(s) are remembered.
     *
     * Use @ref missingParts() to get a list of missing parts.
     * @see idForClass()
     */
    bool checkProject(const QString& singlePartClass = QString());

    class Private;
    Private * const d;

    friend class KexiMainWindow;
    friend class KexiWindow;
    friend class Private;
};

#endif
