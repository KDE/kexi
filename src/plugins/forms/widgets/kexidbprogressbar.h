/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2005 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2012 Oleg Kukharchuk <oleg.kuh@gmail.com>

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

#ifndef KEXIDBPROGRESSBAR_H
#define KEXIDBPROGRESSBAR_H

#include "kexiformutils_export.h"
#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include <formeditor/FormWidgetInterface.h>
#include <QProgressBar>

//! @short A db-aware Progress bar
class KEXIFORMUTILS_EXPORT KexiDBProgressBar : public QProgressBar,
                                               public KexiFormDataItemInterface,
                                               public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)

public:
    explicit KexiDBProgressBar(QWidget *parent = 0);
    virtual ~KexiDBProgressBar();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
    }
    virtual QVariant value() override;
    virtual void setInvalidState(const QString& displayText) override;

    //! \return true if editor's value is null (not empty)
    //! Used for checking if a given constraint within table of form is met.
    virtual bool valueIsNull() override;

    //! \return true if editor's value is empty (not necessary null).
    //! Only few data types can accept "EMPTY" property
    //! (use KDbField::hasEmptyProperty() to check this).
    //! Used for checking if a given constraint within table or form is met.
    virtual bool valueIsEmpty() override;

    /*! \return 'readOnly' flag for this widget. */
    virtual bool isReadOnly() const override;

    /*! \return the view widget of this item, e.g. line edit widget. */
    virtual QWidget* widget() override;

    virtual bool cursorAtStart() override;
    virtual bool cursorAtEnd() override;
    virtual void clear() override;

    virtual void  setEnabled(bool enabled);

public Q_SLOTS:
    inline void setDataSource(const QString &ds) {
        KexiFormDataItemInterface::setDataSource(ds);
    }
    inline void setDataSourcePluginId(const QString &pluginId) {
        KexiFormDataItemInterface::setDataSourcePluginId(pluginId);
    }
    void slotValueChanged();
    virtual void setReadOnly(bool set) override;
protected:
    virtual void setValueInternal(const QVariant& add, bool removeOld) override;

private:
    bool m_invalidState = false;
};

#endif
