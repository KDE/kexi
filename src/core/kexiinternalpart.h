/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004-2006 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIINTERNALPART_H
#define KEXIINTERNALPART_H

#include <QVariant>

#include "kexipartbase.h"

class QDialog;
class KexiWindow;
class KexiView;
class KDbMessageHandler;

/**
 * @short A prototype for Kexi Internal Parts (plugins) implementation.
 *
 * Internal Kexi parts are parts that are not available for users, but loaded
 * internally by application when needed. Example of such part is Relations Window.
 * The internal part instance is unique and has no explicitly stored data.
 * Parts may be able to create widgets or/and dialogs, depending on implementation
 * (createWidgetInstance(), createKexiWindowInstance(), createModalDialogInstance()).
 * Parts can have unique flag set for windows (true by default)
 * - then a dialog created by createModalDialogInstance() is unique.
 */
class KEXICORE_EXPORT KexiInternalPart : public KexiPart::PartBase
{
    Q_OBJECT

public:
    KexiInternalPart(QObject *parent, const QVariantList &list);
    virtual ~KexiInternalPart();

    /*! Creates a new widget instance using part pointed by \a className.
     \a widgetClass is a pseudo class used in case when the part offers more
     than one widget type.
     \a msgHdr is a message handler for displaying error messages.
     \a args is two-way optional argument: it can contain custom options used
     on widget's creation. Depending on implementation, the created widget can write its
     state (e.g. result or status information) back to this argument.
     Created widget will have assigned \a parent widget and \a objName name. */
    Q_REQUIRED_RESULT static QWidget *createWidgetInstance(const QString &className,
                                         const char* widgetClass,
                                         KDbMessageHandler *msgHdr,
                                         QWidget *parent, const char *objName = nullptr,
                                         QMap<QString, QString>* args = nullptr);

    /*! For convenience. */
    Q_REQUIRED_RESULT static QWidget *createWidgetInstance(const QString &className,
                                         KDbMessageHandler *msgHdr,
                                         QWidget *parent, const char *objName = nullptr,
                                         QMap<QString, QString>* args = nullptr);

    /*! Creates a new object instance using part pointed by \a className.
     \a widgetClass is a pseudo class used in case when the part offers more
     than one object type. */
    Q_REQUIRED_RESULT static QObject *createObjectInstance(const QString &className,
                                         const char* objectClass, KDbMessageHandler *msgHdr,
                                         QObject *parent, const char *objName = nullptr,
                                         QMap<QString, QString>* args = nullptr);

    /*! Creates a new KexiWindow instance. If such instance already exists,
     and is unique (see uniqueWindow()) it is just returned.
     The part knows about destroying its window instance (if it is uinque),
     so on another call the window will be created again.
     \a msgHdr is a message handler for displaying error messages.
     The window is assigned to the main window,
     and \a objName name is set. */
    Q_REQUIRED_RESULT static KexiWindow *createKexiWindowInstance(const QString &className,
            KDbMessageHandler *msgHdr, const char *objName = nullptr);

    /*! Creates a new modal dialog instance (QDialog or a subclass).
     If such instance already exists, and is unique (see uniqueWindow())
     it is just returned.
     \a dialogClass is a pseudo class used in case when the part offers more
     than one dialog type.
     \a msgHdr is a message handler for displaying error messages.
     \a args is two-way optional argument: it can contain custom options used
     on widget's creation. Depending on implementation, the created dialog can write its
     state (e.g. result or status information) back to this argument.
     The part knows about destroying its dialog instance, (if it is uinque),
     so on another call the dialog will be created again.
     The dialog is assigned to the main window,
     and \a objName name is set. */
    Q_REQUIRED_RESULT static QDialog *createModalDialogInstance(const QString &className,
            const char* dialogClass, KDbMessageHandler *msgHdr,
            const char *objName = nullptr, QMap<QString, QString>* args = nullptr);

    /*! Adeded For convenience. */
    Q_REQUIRED_RESULT static QDialog *createModalDialogInstance(const QString &className,
            KDbMessageHandler *msgHdr, const char *objName = nullptr,
            QMap<QString, QString>* args = nullptr);

    /*! Executes a command \a commandName (usually nonvisual) using part pointed by \a className.
     The result can be put into the \a args. \return true on successful calling. */
    static bool executeCommand(const QString &className,
                               const char* commandName, QMap<QString, QString>* args = 0);

    /*! \return internal part pointed by \a className. Shouldn't be usable. */
    static KexiInternalPart* part(KDbMessageHandler *msgHdr, const QString &className);

    /*! \return true if the part can create only one (unique) window. */
    bool createsUniqueWindow() const;

    void setCreatesUniqueWindow(bool set);

    /*! \return true if the part creation has been cancelled (eg. by a user)
     so it wasn't an error. Internal part's impelmentation should set it to true when needed.
     False by default. */
    bool cancelled() const;

    void setCancelled(bool set);

protected:
    /*! Used internally */
    KexiWindow *findOrCreateKexiWindow(const char *objName);

    /*! Reimplement this if your internal part has to return objects. */
    Q_REQUIRED_RESULT virtual QObject *createObject(const char* objectClass,
                                  QObject * parent, const char * objName = nullptr,
                                  QMap<QString, QString>* args = nullptr);

    /*! Reimplement this if your internal part has to return widgets
     or QDialog objects. */
    Q_REQUIRED_RESULT virtual QWidget *createWidget(const char* widgetClass,
                                  QWidget * parent, const char * objName = nullptr,
                                  QMap<QString, QString>* args = nullptr);

    /*! Reimplement this if your internal part has to return a view object. */
    Q_REQUIRED_RESULT virtual KexiView *createView(QWidget * parent, const char *objName = nullptr);

    /*! Reimplement this if your internal part has to execute a command \a commandName
     (usually nonvisual). Arguments are put into \a args and the result can be put into the \a args.
     \return true on successful calling. */
    virtual bool executeCommand(const char* commandName, QMap<QString, QString>* args = 0);

private:

    Q_DISABLE_COPY(KexiInternalPart)

    class Private;
    Private* const d;
};

#endif
