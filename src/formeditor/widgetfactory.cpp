/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2009 Jarosław Staniek <staniek@kde.org>

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

#include "widgetfactory.h"

#include <kdebug.h>
#include <klocale.h>
//#ifdef KEXI_KTEXTEDIT
#include <ktextedit.h>
//#else
#include <klineedit.h>
//#endif
#include <kdialog.h>
#include <keditlistbox.h>
#include <kxmlguiclient.h>
#include <kactioncollection.h>

#include "richtextdialog.h"
#include "editlistviewdialog.h"
#include "resizehandle.h"
//#include "formmanager.h"
#include "form.h"
#include "container.h"
#include "objecttree.h"
#include "widgetlibrary.h"
#include "utils.h"
//removed #include "widgetpropertyset.h"
#include "widgetwithsubpropertiesinterface.h"
#include <koproperty/Property.h>
#include <koproperty/Set.h>
#include <kexiutils/utils.h>

using namespace KFormDesigner;

///// Widget Info //////////////////////////

WidgetInfo::WidgetInfo(WidgetFactory *f)
        : m_inheritedClass(0)
        , m_overriddenAlternateNames(0)
        , m_factory(f)
        , m_propertiesWithDisabledAutoSync(0)
        , m_customTypesForProperty(0)
{
}

WidgetInfo::WidgetInfo(WidgetFactory *f, const char* parentFactoryName,
                       const char* inheritedClassName)
        : m_parentFactoryName(QByteArray("kformdesigner_") + parentFactoryName)
        , m_inheritedClassName(inheritedClassName)
        , m_inheritedClass(0)
        , m_overriddenAlternateNames(0)
        , m_factory(f)
        , m_propertiesWithDisabledAutoSync(0)
        , m_customTypesForProperty(0)
{
    m_class = inheritedClassName;
}

WidgetInfo::~WidgetInfo()
{
    delete m_overriddenAlternateNames;
    delete m_propertiesWithDisabledAutoSync;
    delete m_customTypesForProperty;
}

void WidgetInfo::addAlternateClassName(const QByteArray& alternateName, bool override)
{
    m_alternateNames += alternateName;
    if (override) {
        if (!m_overriddenAlternateNames)
            m_overriddenAlternateNames = new QSet<QByteArray>;
        m_overriddenAlternateNames->insert(alternateName);
    } else {
        if (m_overriddenAlternateNames)
            m_overriddenAlternateNames->remove(alternateName);
    }
}

bool WidgetInfo::isOverriddenClassName(const QByteArray& alternateName) const
{
    return m_overriddenAlternateNames && m_overriddenAlternateNames->contains(alternateName);
}

void WidgetInfo::setAutoSyncForProperty(const char *propertyName, tristate flag)
{
    if (!m_propertiesWithDisabledAutoSync) {
        if (~flag)
            return;
        m_propertiesWithDisabledAutoSync = new QHash<QByteArray, tristate>;
    }

    if (~flag) {
        m_propertiesWithDisabledAutoSync->remove(propertyName);
    } else {
        m_propertiesWithDisabledAutoSync->insert(propertyName, flag);
    }
}

tristate WidgetInfo::autoSyncForProperty(const char *propertyName) const
{
    if (!m_propertiesWithDisabledAutoSync)
        return cancelled;
    tristate flag = m_propertiesWithDisabledAutoSync->value(propertyName);
    return flag;
}

void WidgetInfo::setCustomTypeForProperty(const char *propertyName, int type)
{
    if (!propertyName || type == (int)KoProperty::Auto)
        return;
    if (!m_customTypesForProperty) {
        m_customTypesForProperty = new QHash<QByteArray, int>();
    }
    m_customTypesForProperty->remove(propertyName);
    m_customTypesForProperty->insert(propertyName, type);
}

int WidgetInfo::customTypeForProperty(const char *propertyName) const
{
    if (!m_customTypesForProperty || !m_customTypesForProperty->contains(propertyName))
        return KoProperty::Auto;
    return m_customTypesForProperty->value(propertyName);
}

///// InlineEditorCreationArguments //////////////////////////

WidgetFactory::InlineEditorCreationArguments::InlineEditorCreationArguments(
    const QByteArray& _classname, QWidget *_widget, Container *_container)
    : classname(_classname), widget(_widget), container(_container), 
      geometry(_widget ? _widget->geometry() : QRect()),
      alignment( Qt::AlignLeft ),
      backgroundMode( Qt::NoBackground ),
      useFrame( false ), multiLine( false ), execute( true )
{
}

///// Widget Factory //////////////////////////

WidgetFactory::WidgetFactory(QObject *parent, const char *name)
        : QObject(parent)
{
    setObjectName(QString("kformdesigner_") + name);
    m_showAdvancedProperties = true;
    m_hiddenClasses = 0;
    m_guiClient = 0;
}

WidgetFactory::~WidgetFactory()
{
    qDeleteAll(m_classesByName);
    delete m_hiddenClasses;
}

void WidgetFactory::addClass(WidgetInfo *w)
{
    WidgetInfo *oldw = m_classesByName.value(w->className());
    if (oldw == w)
        return;
    if (oldw) {
        kWarning() << "class with name '"
            << w->className()
            << "' already exists for factory '" << objectName() << "'";
        return;
    }
    m_classesByName.insert(w->className(), w);
}

void WidgetFactory::hideClass(const char *classname)
{
    if (!m_hiddenClasses)
        m_hiddenClasses = new QSet<QByteArray>;
    m_hiddenClasses->insert(QByteArray(classname).toLower());
}


void WidgetFactory::disableFilter(QWidget *w, Container *container)
{
    container->form()->disableFilter(w, container);
}

bool WidgetFactory::editList(QWidget *w, QStringList &list) const
{
    KDialog dialog(w->topLevelWidget());
    dialog.setObjectName("stringlist_dialog");
    dialog.setModal(true);
    dialog.setCaption(i18n("Edit List of Items"));
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);

    KEditListBox *edit = new KEditListBox(
        i18n("Contents of %1", w->objectName()), &dialog, "editlist");
    dialog.setMainWidget(edit);
    edit->insertStringList(list);

    if (dialog.exec() == QDialog::Accepted) {
        list = edit->items();
        return true;
    }
    return false;
}

bool WidgetFactory::editRichText(QWidget *w, QString &text) const
{
    RichTextDialog dlg(w, text);
    if (dlg.exec() == QDialog::Accepted) {
        text = dlg.text();
        return true;
    }
    return false;
}

#ifndef KEXI_FORMS_NO_LIST_WIDGET
void
WidgetFactory::editListWidget(QListWidget *listwidget) const
{
    EditListViewDialog dlg(((QWidget*)listwidget)->topLevelWidget());
//! @todo
    //dlg.exec(listview);
}
#endif

void WidgetFactory::changeProperty(Form *form, QWidget *widget, const char *name, const QVariant &value)
{
    if (form->selectedWidget()) { // single selection
        form->propertySet().changePropertyIfExists(name, value);
        widget->setProperty(name, value);
    }
    else {
        // If eg multiple labels are selected, 
        // we only want to change the text of one of them (the one the user cliked on)
        if (widget) {
            widget->setProperty(name, value);
        }
        else {
            form->selectedWidgets()->first()->setProperty(name, value);
        }
    }
}

/*
void
WidgetFactory::addPropertyDescription(Container *container, const char *prop, const QString &desc)
{
  WidgetPropertySet *buff = container->form()->manager()->buffer();
  buff->addPropertyDescription(prop, desc);
}

void
WidgetFactory::addValueDescription(Container *container, const char *value, const QString &desc)
{
  WidgetPropertySet *buff = container->form()->manager()->buffer();
  buff->addValueDescription(value, desc);
}*/

bool
WidgetFactory::isPropertyVisible(const QByteArray &classname, QWidget *w,
                                 const QByteArray &property, bool multiple, bool isTopLevel)
{
    if (multiple) {
        return property == "font" || property == "paletteBackgroundColor" || property == "enabled"
               || property == "paletteForegroundColor" || property == "cursor" 
               || property == "paletteBackgroundPixmap";
    }

    return isPropertyVisibleInternal(classname, w, property, isTopLevel);
}

bool
WidgetFactory::isPropertyVisibleInternal(const QByteArray &, QWidget *w,
        const QByteArray &property, bool isTopLevel)
{
    Q_UNUSED(w);

#ifdef KEXI_NO_CURSOR_PROPERTY
//! @todo temporary unless cursor works properly in the Designer
    if (property == "cursor")
        return false;
#endif

    if (!isTopLevel
            && (property == "windowTitle" || property == "windowIcon" || property == "sizeIncrement" || property == "windowIconText")) {
        // don't show these properties for a non-toplevel widget
        return false;
    }
    return true;
}

bool
WidgetFactory::propertySetShouldBeReloadedAfterPropertyChange(
    const QByteArray& classname, QWidget *w, const QByteArray& property)
{
    Q_UNUSED(classname)
    Q_UNUSED(w)
    Q_UNUSED(property)
    return false;
}

void
WidgetFactory::resizeEditor(QWidget *, QWidget *, const QByteArray&)
{
}

bool
WidgetFactory::clearWidgetContent(const QByteArray &, QWidget *)
{
    return false;
}

bool WidgetFactory::changeInlineText(Form *form, QWidget *widget,
                                     const QString& text, QString &oldText)
{
    oldText = widget->property("text").toString();
    changeProperty(form, widget, "text", text);
    return true;
}

bool
WidgetFactory::readSpecialProperty(const QByteArray &, QDomElement &, QWidget *, ObjectTreeItem *)
{
    return false;
}

bool
WidgetFactory::saveSpecialProperty(const QByteArray &, const QString &, const QVariant&, QWidget *, QDomElement &,  QDomDocument &)
{
    return false;
}

bool WidgetFactory::inheritsFactories()
{
    foreach (WidgetInfo *winfo, m_classesByName) {
        if (!winfo->parentFactoryName().isEmpty())
            return true;
    }
    return false;
}

#if 0 // 2.0
void WidgetFactory::setEditor(QWidget *widget, QWidget *editor)
{
    if (!widget)
        return;
    WidgetInfo *winfo = m_classesByName.value(widget->metaObject()->className());
    if (!winfo || winfo->parentFactoryName().isEmpty()) {
        m_editor = editor;
    } else {
        WidgetFactory *f = m_library->factory(winfo->parentFactoryName());
        if (f != this)
            f->setEditor(widget, editor);
        m_editor = editor; //keep a copy
    }
}
#endif

#if 0 // 2.0
QWidget *WidgetFactory::editor(QWidget *widget) const
{
    if (!widget)
        return 0;
    WidgetInfo *winfo = m_classesByName.value(widget->metaObject()->className());
    if (!winfo || winfo->parentFactoryName().isEmpty()) {
        return m_editor;
    } else {
        WidgetFactory *f = m_library->factoryForClassName(
                               widget->metaObject()->className());
        if (f != this)
            return f->editor(widget);
        return m_editor;
    }
}
#endif

#if 0 // 2.0
void WidgetFactory::setWidget(QWidget *widget, Container* container)
{
    WidgetInfo *winfo = widget
        ? m_classesByName.value(widget->metaObject()->className()) : 0;
    if (winfo && !winfo->parentFactoryName().isEmpty()) {
        WidgetFactory *f = m_library->factory(winfo->parentFactoryName());
        if (f != this)
            f->setWidget(widget, container);
    }
    m_widget = widget; //keep a copy
    m_container = container;
}
#endif

#if 0 // 2.0
QWidget *WidgetFactory::widget() const
{
    return m_widget;
}
#endif

void WidgetFactory::setInternalProperty(const QByteArray& classname, const QByteArray& property,
                                        const QString& value)
{
    m_internalProp.insert(classname+":"+property, value);
}

void WidgetFactory::setPropertyOptions(KoProperty::Set& set, const WidgetInfo& info, QWidget *w)
{
    Q_UNUSED(set)
    Q_UNUSED(info)
    Q_UNUSED(w)
    //nothing
}

#include "widgetfactory.moc"
