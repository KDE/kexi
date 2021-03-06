/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2009 Jarosław Staniek <staniek@kde.org>

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
#ifndef KEXIDBFORM_H
#define KEXIDBFORM_H

#include "kexiformutils_export.h"
#include <formeditor/form.h>
#include <formeditor/FormWidget.h>
#include <formeditor/FormWidgetInterface.h>
#include <widget/dataviewcommon/kexiformdataiteminterface.h>

class QEvent;
class QDropEvent;
class QDragMoveEvent;

class KexiDataAwareObjectInterface;
class KexiFormScrollView;

//! @short A DB-aware form widget, acting as form's toplevel widget
class KEXIFORMUTILS_EXPORT KexiDBForm : public QWidget,
                                        public KFormDesigner::FormWidget,
                                        public KexiFormDataItemInterface,
                                        public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)
    Q_PROPERTY(bool autoTabStops READ autoTabStops WRITE setAutoTabStops)
    //original "size" property is not designable, so here's a custom (not storable) replacement
    Q_PROPERTY(QSize sizeInternal READ sizeInternal WRITE resizeInternal STORED false)
public:
    KexiDBForm(QWidget *parent, KexiDataAwareObjectInterface* dataAwareObject);
    virtual ~KexiDBForm();

    KexiDataAwareObjectInterface* dataAwareObject() const;

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
    }

    //! no effect
    virtual QVariant value() override {
        return QVariant();
    }

    virtual void setInvalidState(const QString& displayText) override;

    bool autoTabStops() const;

    QList<QWidget*>* orderedFocusWidgets() const;

    QList<QWidget*>* orderedDataAwareWidgets() const;

    void updateTabStopsOrder(KFormDesigner::Form* form);

    void updateTabStopsOrder();

    virtual bool valueIsNull() override;
    virtual bool valueIsEmpty() override;
    virtual bool isReadOnly() const override;
    virtual QWidget* widget() override;
    virtual bool cursorAtStart() override;
    virtual bool cursorAtEnd() override;
    virtual void clear() override;

    bool isPreviewing() const;

public Q_SLOTS:
    void setAutoTabStops(bool set);
    inline void setDataSource(const QString &ds) {
        KexiFormDataItemInterface::setDataSource(ds);
    }
    inline void setDataSourcePluginId(const QString &pluginId) {
        KexiFormDataItemInterface::setDataSourcePluginId(pluginId);
    }

    //! This implementation just disables read only widget
    virtual void setReadOnly(bool readOnly) override;

    //! @internal for sizeInternal property
    QSize sizeInternal() const {
        return QWidget::size();
    }

    //! @internal for sizeInternal property
    void resizeInternal(const QSize& s) {
        QWidget::resize(s);
    }

Q_SIGNALS:
    void handleDragMoveEvent(QDragMoveEvent *e);
    void handleDropEvent(QDropEvent *e);

protected:
    virtual bool eventFilter(QObject * watched, QEvent * e) override;

    virtual void paintEvent(QPaintEvent *e) override;

    //! no effect
    virtual void setValueInternal(const QVariant&, bool) override {}

    //! Used to emit handleDragMoveEvent() signal needed to control dragging over the container's surface
    virtual void dragMoveEvent(QDragMoveEvent *e) override;

    //! Used to emit handleDropEvent() signal needed to control dropping on the container's surface
    virtual void dropEvent(QDropEvent *e) override;

    //! called from KexiFormScrollView::initDataContents()
    void updateReadOnlyFlags();

    //! Points to a currently edited data item.
    //! It is cleared when the focus is moved to other
    KexiFormDataItemInterface *editedItem;

    class Private;
    Private * const d;

    friend class KexiFormScrollView;
};

#endif
