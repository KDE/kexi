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

#include <KEditListWidget>
#include <KLocalizedString>

#include "richtextdialog.h"
#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
# include "editlistviewdialog.h"
#endif

#include "form.h"
#include "container.h"
#include "objecttree.h"
#include "widgetlibrary.h"
#include "WidgetInfo.h"
#include "widgetwithsubpropertiesinterface.h"
#include "utils.h"
#include <kexiutils/utils.h>

#include <KProperty>
#include <KPropertySet>

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDebug>

using namespace KFormDesigner;

InternalPropertyHandlerInterface::InternalPropertyHandlerInterface()
{
}

InternalPropertyHandlerInterface::~InternalPropertyHandlerInterface()
{
}

///// InlineEditorCreationArguments //////////////////////////

WidgetFactory::InlineEditorCreationArguments::InlineEditorCreationArguments(
    const QByteArray& _classname, QWidget *_widget, Container *_container)
    : classname(_classname), widget(_widget), container(_container),
      geometry(_widget ? _widget->geometry() : QRect()),
      alignment( Qt::AlignLeft ),
      useFrame( false ), multiLine( false ), execute( true ), transparentBackground( false )
{
}

///// Widget Factory //////////////////////////

class Q_DECL_HIDDEN WidgetFactory::Private
{
public:
    Private();
    ~Private();

    WidgetLibrary *library;

    QHash<QByteArray, WidgetInfo*> classesByName;
    QSet<QByteArray>* hiddenClasses;

    //! i18n stuff
    QHash<QByteArray, QString> propDesc;
    QHash<QByteArray, QString> propValDesc;
    //! internal properties
    QHash<QByteArray, QVariant> internalProp;

    /*! flag useful to decide whether to hide some properties.
     It's value is inherited from WidgetLibrary. */
    bool advancedPropertiesVisible;
};

WidgetFactory::Private::Private()
    : hiddenClasses(0), advancedPropertiesVisible(true)
{

}

WidgetFactory::Private::~Private()
{
    qDeleteAll(classesByName);
    delete hiddenClasses;
}

WidgetFactory::WidgetFactory(QObject *parent)
    : QObject(parent), d(new Private())
{
}

WidgetFactory::~WidgetFactory()
{
    delete d;
}

void WidgetFactory::addClass(WidgetInfo *w)
{
    WidgetInfo *oldw = d->classesByName.value(w->className());
    if (oldw == w)
        return;
    if (oldw) {
        qWarning() << "class with name '"
            << w->className()
            << "' already exists for factory '" << objectName() << "'";
        return;
    }
    d->classesByName.insert(w->className(), w);
}

void WidgetFactory::hideClass(const char *classname)
{
    if (!d->hiddenClasses)
        d->hiddenClasses = new QSet<QByteArray>;
    d->hiddenClasses->insert(QByteArray(classname).toLower());
}

QHash<QByteArray, WidgetInfo*> WidgetFactory::classes() const
{
    return d->classesByName;
}

void WidgetFactory::disableFilter(QWidget *w, Container *container)
{
    container->form()->disableFilter(w, container);
}

bool WidgetFactory::editList(QWidget *w, QStringList &list) const
{
    //! @todo KEXI3 port to QDialog
#if 1
    Q_UNUSED(w);
    Q_UNUSED(list);
#else
    QDialog dialog(w->topLevelWidget());
    dialog.setObjectName("stringlist_dialog");
    dialog.setModal(true);
    dialog.setWindowTitle(xi18nc("@title:window", "Edit Contents of %1", w->objectName()));
    dialog.setButtons(QDialog::Ok | QDialog::Cancel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    dialog.setLayout(mainLayout);
    KEditListWidget *edit = new KEditListWidget(&dialog);
    edit->setObjectName("editlist");
    edit->insertStringList(list);
    mainLayout->addWidget(edit);

    // buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted) {
        list = edit->items();
        return true;
    }
#endif
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

#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
void
WidgetFactory::editListWidget(QListWidget *listwidget) const
{
    EditListViewDialog dlg(static_cast<QWidget*>(listwidget)->topLevelWidget());
    Q_UNUSED(dlg)
//! @todo
    //dlg.exec(listview);
}
#endif

void WidgetFactory::changeProperty(Form *form, QWidget *widget, const char *name, const QVariant &value)
{
    if (form->selectedWidget()) { // single selection
        form->propertySet()->changePropertyIfExists(name, value);
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

    if (property == "objectName") { // name is available in the KexiObjectInfoWidget
        return false;
    }
#ifndef KEXI_FORM_CURSOR_PROPERTY_SUPPORT
//! @todo temporary unless cursor works properly in the Designer
    if (property == "cursor")
        return false;
#endif
    if (property == "acceptDrops" || property == "inputMethodHints")
        return false;

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
                                     const QString& text, QString *oldText)
{
    if (oldText) {
        *oldText = widget->property("text").toString();
    }
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
    foreach (WidgetInfo *winfo, d->classesByName) {
        if (!winfo->parentFactoryName().isEmpty())
            return true;
    }
    return false;
}

void WidgetFactory::setPropertyOptions(KPropertySet& set, const WidgetInfo& info, QWidget *w)
{
    Q_UNUSED(set)
    Q_UNUSED(info)
    Q_UNUSED(w)
    //nothing
}

ObjectTreeItem* WidgetFactory::selectableItem(ObjectTreeItem* item)
{
    return item;
}

void WidgetFactory::setInternalProperty(const QByteArray &classname, const QByteArray &property, const QVariant &value)
{
    d->internalProp.insert(classname + ":" + property, value);
}

QVariant WidgetFactory::internalProperty(const QByteArray &classname, const QByteArray &property) const
{
    return d->internalProp.value(classname + ":" + property);
}

QString WidgetFactory::propertyDescription(const char* name) const
{
    return d->propDesc.value(name);
}

QString WidgetFactory::valueDescription(const char* name) const
{
    return d->propValDesc.value(name);
}

WidgetInfo* WidgetFactory::widgetInfoForClassName(const char* classname)
{
    return d->classesByName.value(classname);
}

const QSet<QByteArray> *WidgetFactory::hiddenClasses() const
{
    return d->hiddenClasses;
}

WidgetLibrary* WidgetFactory::library()
{
    return d->library;
}

bool WidgetFactory::advancedPropertiesVisible() const
{
    return d->advancedPropertiesVisible;
}

void WidgetFactory::setLibrary(WidgetLibrary* library)
{
    d->library = library;
}

void WidgetFactory::setAdvancedPropertiesVisible(bool set)
{
    d->advancedPropertiesVisible = set;
}

void WidgetFactory::setPropertyDescription(const char* property, const QString &description)
{
    d->propDesc.insert(property, description);
}

void WidgetFactory::setValueDescription(const char *valueName, const QString &description)
{
    d->propValDesc.insert(valueName, description);
}

