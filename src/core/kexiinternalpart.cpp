/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
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

#include "kexiinternalpart.h"
#include "kexipartmanager.h"
#include "KexiWindow.h"
#include "KexiView.h"
#include "KexiMainWindowIface.h"
#include <kexi_global.h>

#include <KDbMessageHandler>

#include <KLocalizedString>

#include <QDebug>
#include <QDialog>

//----------------------------------------------

class Q_DECL_HIDDEN KexiInternalPart::Private
{
public:
    Private();
    ~Private();

    //! Unique widget - we're using guarded ptr for the widget so can know if it has been closed
    QPointer<QWidget> uniqueWidget;
    bool uniqueWindow; //!< true if createWidgetInstance() should return only one window
    bool cancelled; //!< Used in cancelled()
};

KexiInternalPart::Private::Private() : uniqueWindow(true)
                                       ,cancelled(false)
{
}

KexiInternalPart::Private::~Private()
{
}

KexiInternalPart::KexiInternalPart(QObject *parent, const QVariantList &list)
        : KexiPart::PartBase(parent, list)
        , d(new Private)
{
}

KexiInternalPart::~KexiInternalPart()
{
    delete d;
}

//static
KexiInternalPart* KexiInternalPart::part(KDbMessageHandler *msgHdr, const QString &className)
{
    KexiInternalPart *part = Kexi::partManager().internalPartForPluginId(className);
    if (!part) {
        if (msgHdr) {
            msgHdr->showErrorMessage(Kexi::partManager().result());
        }
    }
    return part;
}

//static
QObject* KexiInternalPart::createObjectInstance(const QString &className,
        const char* objectClass, KDbMessageHandler *msgHdr,
        QObject *parent, const char *objName, QMap<QString, QString>* args)
{
    KexiInternalPart *part = KexiInternalPart::part(msgHdr, className);
    if (!part) {
        return nullptr; //fatal!
    }
    return part->createObject(objectClass, parent,
                              objName ? objName : qPrintable(className), args);
}

//static
QWidget* KexiInternalPart::createWidgetInstance(const QString &className,
        const char* widgetClass, KDbMessageHandler *msgHdr,
        QWidget *parent, const char *objName, QMap<QString, QString>* args)
{
    KexiInternalPart *part = KexiInternalPart::part(msgHdr, className);
    if (!part)
        return 0; //fatal!
    return part->createWidget(widgetClass, parent,
                              objName ? objName : qPrintable(className), args);
}

KexiWindow* KexiInternalPart::findOrCreateKexiWindow(
    const char *objName)
{
    if (d->uniqueWindow && !d->uniqueWidget.isNull())
        return dynamic_cast<KexiWindow*>((QWidget*)d->uniqueWidget);
    KexiWindow * wnd = new KexiWindow();
    KexiView *view = createView(0, objName);
    if (!view)
        return 0;

    if (d->uniqueWindow)
        d->uniqueWidget = wnd; //recall unique!
    wnd->addView(view);
    wnd->setWindowTitle(view->windowTitle());
    wnd->resize(view->sizeHint());
    wnd->setMinimumSize(view->minimumSizeHint().width(), view->minimumSizeHint().height());
    wnd->setId(KexiMainWindowIface::global()->project()->generatePrivateID());
    wnd->registerWindow();
    return wnd;
}

//static
QWidget* KexiInternalPart::createWidgetInstance(const QString &className,
                                                KDbMessageHandler *msgHdr,
                                                QWidget *parent, const char *objName,
                                                QMap<QString, QString>* args)
{
    return createWidgetInstance(className, 0, msgHdr, parent, objName, args);
}

//static
KexiWindow* KexiInternalPart::createKexiWindowInstance(
    const QString &className,
    KDbMessageHandler *msgHdr, const char *objName)
{
    KexiInternalPart *part = KexiInternalPart::part(msgHdr, className);
    if (!part) {
        qWarning() << "!part";
        return 0; //fatal!
    }
    return part->findOrCreateKexiWindow(objName ? objName : qPrintable(className));
}

//static
QDialog* KexiInternalPart::createModalDialogInstance(const QString &className,
        const char* dialogClass, KDbMessageHandler *msgHdr,
        const char *objName, QMap<QString, QString>* args)
{
    KexiInternalPart *part = KexiInternalPart::part(msgHdr, className);
    if (!part) {
        qWarning() << "!part";
        return 0; //fatal!
    }
    QWidget *w;
    if (part->createsUniqueWindow() && !part->d->uniqueWidget.isNull())
        w = part->d->uniqueWidget;
    else
        w = part->createWidget(dialogClass, KexiMainWindowIface::global()->thisWidget(),
                               objName ? objName : qPrintable(className), args);

    QDialog* dialog = qobject_cast<QDialog*>(w);
    if (dialog) {
        if (part->createsUniqueWindow())
            part->d->uniqueWidget = w;
        return dialog;
    }
    //sanity
    if (!(part->createsUniqueWindow() && !part->d->uniqueWidget.isNull()))
        delete w;
    return 0;
}

//static
QDialog* KexiInternalPart::createModalDialogInstance(const QString &className,
                                                     KDbMessageHandler *msgHdr, const char *objName,
                                                     QMap<QString, QString>* args)
{
    return createModalDialogInstance(className, 0, msgHdr, objName, args);
}

//static
bool KexiInternalPart::executeCommand(const QString &className,
                                      const char* commandName, QMap<QString, QString>* args)
{
    KexiInternalPart *part = KexiInternalPart::part(0, className);
    if (!part) {
        qWarning() << "!part";
        return 0; //fatal!
    }
    return part->executeCommand(commandName, args);
}

QObject* KexiInternalPart::createObject(const char* objectClass,
                              QObject * parent, const char * objName,
                              QMap<QString, QString>* args)
{
    Q_UNUSED(objectClass);
    Q_UNUSED(parent);
    Q_UNUSED(objName);
    Q_UNUSED(args);
    return 0;
}

QWidget* KexiInternalPart::createWidget(const char* widgetClass,
                                        QWidget * parent, const char * objName, QMap<QString, QString>* args)
{
    Q_UNUSED(widgetClass);
    Q_UNUSED(parent);
    Q_UNUSED(objName);
    Q_UNUSED(args);
    return 0;
}

KexiView* KexiInternalPart::createView(QWidget * parent,
                                       const char * objName)
{
    Q_UNUSED(parent);
    Q_UNUSED(objName);
    return 0;
}

bool KexiInternalPart::executeCommand(const char* commandName,
                                      QMap<QString, QString>* args)
{
    Q_UNUSED(commandName);
    Q_UNUSED(args);
    return false;
}

bool KexiInternalPart::createsUniqueWindow() const
{
    return d->uniqueWindow;
}

void KexiInternalPart::setCreatesUniqueWindow(bool set)
{
    d->uniqueWindow = set;
}

bool KexiInternalPart::cancelled() const
{
    return d->cancelled;
}

void KexiInternalPart::setCancelled(bool set)
{
    d->cancelled = set;
}

