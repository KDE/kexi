/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2005-2008 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPARTITEM_H
#define KEXIPARTITEM_H

#include <QHash>
#include <kexi_global.h>

namespace KexiPart
{

class Info;

/*!
 @short Information about a single object that can be instantiated using Kexi Part

 KexiPart::Item stores:
  - identifier (low-level name, for example: table name)
  - part class, eg. "org.kexi-project.table"
  - caption (visible, i18n'd hight level name, eg. table or query title)
*/
class KEXICORE_EXPORT Item
{
public:
    Item();
    ~Item();

    int identifier() const {
        return m_id;
    }
    void setIdentifier(int id) {
        m_id = id;
    }

/*    QString mimeType() const {
        return m_mime;
    }
    void setMimeType(const QString &mime) {
        m_mime = mime;
    }*/
    QString partClass() const {
        return m_class;
    }
    void setPartClass(const QString &_class) {
        m_class = _class;
    }

    QString name() const {
        return m_name;
    }
    void setName(const QString &name) {
        m_name = name;
    }

    QString caption() const {
        return m_caption;
    }
    void setCaption(const QString &c) {
        m_caption = c;
    }

    QString description() const {
        return m_desc;
    }
    void setDescription(const QString &d) {
        m_desc = d;
    }

    /*! \return "neverSaved" flag for this item what mean
     that is used when new item is created in-memory-only,
     so we need to indicate for KexiProject about that state.
     By default this flag is false.
     Used by KexiMainWindow::newObject(). */
    bool neverSaved() const {
        return m_neverSaved;
    }

    /*! \sa neverSaved().
     Used by KexiMainWindow::newObject(). */
    void setNeverSaved(bool set) {
        m_neverSaved = set;
    }

    bool isNull() const {
        return m_id == 0;
    }

    //! \return caption if not empty, else returns name.
    inline QString captionOrName() const {
        return m_caption.isEmpty() ? m_name : m_caption;
    }

private:
    QString m_class;
    QString m_name;
    QString m_caption;
    QString m_desc;
    int m_id;
bool m_neverSaved : 1;

    class Private;
    Private * const d;
};

//! Item dict which destroys KexiPart::Item items on destruction.
class KEXICORE_EXPORT ItemDict : public QHash<int, KexiPart::Item*>
{
public:
    ItemDict();
    ~ItemDict();
};

//typedef QHash<int, KexiPart::Item*>::iterator ItemDictIterator;
typedef QList<KexiPart::Item*>::iterator ItemListIterator;

/*!
 @short Part item list with special sorting method (by item name).

 Such a list is returend by KexiProject::getSortedItems(KexiPart::ItemList& list, KexiPart::Info *i);
 so you can call sort() on the list to sort it by item name.
*/
class KEXICORE_EXPORT ItemList : public QList<KexiPart::Item*>
{
public:
    ItemList();

    //! Sorts the list by item names.
    void sort();
};

}

#endif
