/* This file is part of the KDE project
   Copyright (C) 2012-2013 Oleg Kukharchuk <oleg.kuh@gmail.com>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2006 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDBCOMMANDLINKBUTTON_H
#define KEXIDBCOMMANDLINKBUTTON_H

#include "kexiformutils_export.h"
#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include <kexiutils/KexiCommandLinkButton.h>
#include <formeditor/kexiformeventhandler.h>
#include <formeditor/FormWidgetInterface.h>

class KexiDBCommandLinkButtonPrivate;

//! @short Push Button widget for Kexi forms
class KEXIFORMUTILS_EXPORT KexiDBCommandLinkButton : public KexiCommandLinkButton,
                                                     public KexiFormDataItemInterface,
                                                     public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString onClickAction READ onClickAction WRITE setOnClickAction)
    Q_PROPERTY(QString onClickActionOption READ onClickActionOption WRITE setOnClickActionOption)
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)

public:
    KexiDBCommandLinkButton(const QString &text, const QString &description, QWidget * parent = 0);
    virtual ~KexiDBCommandLinkButton();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
    }

    virtual QVariant value() override;

    virtual bool valueIsNull() override;

    virtual bool valueIsEmpty() override;

    //! always false
    virtual bool cursorAtStart() override;

    //! always false
    virtual bool cursorAtEnd() override;

    virtual void clear() override;

    virtual void setInvalidState(const QString& displayText) override;

    virtual bool isReadOnly() const override;
public Q_SLOTS:
    //! action string for "on click" event
    //! @see KexiFormPart::slotAssignAction()
    //! @see KexiFormEventAction::ActionData
    QString onClickAction() const;

    void setOnClickAction(const QString& actionString);

    /*! action option allowing to select whether the object should be opened
     data view mode or printed, etc.
     @see KexiFormPart::slotAssignAction()
     @see KexiFormEventAction::ActionData */
    QString onClickActionOption() const;

    void setOnClickActionOption(const QString& option);

    //! Sets the datasource to \a ds
    inline void setDataSource(const QString &ds) {
        KexiFormDataItemInterface::setDataSource(ds);
    }

    inline void setDataSourcePluginId(const QString &pluginId) {
        KexiFormDataItemInterface::setDataSourcePluginId(pluginId);
    }
    virtual void setReadOnly(bool readOnly) override;

protected:
    //! Sets value \a value for a widget.
    virtual void setValueInternal(const QVariant& add, bool removeOld) override;

    friend class KexiDBCommanLinkButtonPrivate;
    KexiDBCommandLinkButtonPrivate * const d;
};

#endif
