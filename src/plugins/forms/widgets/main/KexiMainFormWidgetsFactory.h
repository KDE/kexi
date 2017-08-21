/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIMAINFORMWIDGETSFACTORY_H
#define KEXIMAINFORMWIDGETSFACTORY_H

#include "kexidbfactorybase.h"

class QAction;

//! Kexi Factory for data-aware widgets
//! @todo merge with KexiStandardFormWidgetsFactory
class KexiMainFormWidgetsFactory : public KexiDBFactoryBase
{
    Q_OBJECT

public:
    KexiMainFormWidgetsFactory(QObject *parent, const QVariantList &);
    virtual ~KexiMainFormWidgetsFactory();

    QWidget *createWidget(const QByteArray &classname, QWidget *parent, const char *name,
                          KFormDesigner::Container *container,
                          CreateWidgetOptions options = DefaultOptions) override Q_REQUIRED_RESULT;

    virtual void createCustomActions(KActionCollection* col);
    virtual bool createMenuActions(const QByteArray &classname, QWidget *w, QMenu *menu,
                                   KFormDesigner::Container *container);
    virtual bool startInlineEditing(InlineEditorCreationArguments& args);
    virtual bool previewWidget(const QByteArray &, QWidget *, KFormDesigner::Container *);
    virtual bool clearWidgetContent(const QByteArray &classname, QWidget *w);

    //! Moved into public for EditRichTextAction
    bool editRichText(QWidget *w, QString &text) const
    {
        return KexiDBFactoryBase::editRichText(w, text);
    }

    //! Moved into public for EditRichTextAction
    void changeProperty(KFormDesigner::Form *form, QWidget *widget, const char *name,
        const QVariant &value)
    {
        KexiDBFactoryBase::changeProperty(form, widget, name, value);
    }

    bool readSpecialProperty(const QByteArray &classname, QDomElement &node,
                             QWidget *w, KFormDesigner::ObjectTreeItem *item)  override;
    bool saveSpecialProperty(const QByteArray &classname, const QString &name,
                             const QVariant &value, QWidget *w,
                             QDomElement &parentNode, QDomDocument &parent) override;
    void setPropertyOptions(KPropertySet& set, const KFormDesigner::WidgetInfo& info, QWidget *w) override;

protected Q_SLOTS:
    void slotImageBoxIdChanged(long id); /*KexiBLOBBuffer::Id_t*/
    void reorderTabs(int oldpos, int newpos);

protected:
    KFormDesigner::ObjectTreeItem* selectableItem(KFormDesigner::ObjectTreeItem* item);
    virtual bool changeInlineText(KFormDesigner::Form *form, QWidget *widget,
        const QString &text, QString &oldText);
    virtual void resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &classname);

    virtual bool isPropertyVisibleInternal(const QByteArray& classname, QWidget *w,
                                           const QByteArray& property, bool isTopLevel);

    //! Sometimes property sets should be reloaded when a given property value changed.
    //! @todo this does not seem to work in Kexi 2.x
    virtual bool propertySetShouldBeReloadedAfterPropertyChange(const QByteArray& classname, QWidget *w,
            const QByteArray& property);

    QAction * m_assignAction;
};

#endif
