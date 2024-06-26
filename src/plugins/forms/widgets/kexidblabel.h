/* This file is part of the KDE project
   Copyright (C) 2005 Christian Nitschkowski <segfault_ii@web.de>
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDBLABEL_H
#define KEXIDBLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QPaintEvent>
#include <QShowEvent>
#include <QResizeEvent>

#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include "kexidbtextwidgetinterface.h"
#include <widget/utils/kexidisplayutils.h>
#include <formeditor/FormWidgetInterface.h>
#include <kexi_global.h>

class QPainter;

//! @short An extended, data-aware, read-only text label.
class KEXIFORMUTILS_EXPORT KexiDBLabel : public QLabel,
                                         protected KexiDBTextWidgetInterface,
                                         public KexiFormDataItemInterface,
                                         public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap DESIGNABLE false)
    Q_PROPERTY(bool scaledContents READ hasScaledContents WRITE setScaledContents DESIGNABLE false)
    Q_PROPERTY(QColor frameColor READ frameColor WRITE setFrameColor)

public:
    explicit KexiDBLabel(QWidget *parent = 0, Qt::WindowFlags f = {});
    explicit KexiDBLabel(const QString& text, QWidget *parent = 0, Qt::WindowFlags f = {});
    virtual ~KexiDBLabel();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
    }

    virtual QVariant value() override;

    virtual void setInvalidState(const QString& displayText) override;

    virtual bool valueIsNull() override;

    virtual bool valueIsEmpty() override;

    //! always true
    virtual bool isReadOnly() const override;

    virtual QWidget* widget() override;

    //! always false
    virtual bool cursorAtStart() override;

    //! always false
    virtual bool cursorAtEnd() override;

    virtual void clear() override;

    //! used to catch setIndent(), etc.
    virtual bool setProperty(const char * name, const QVariant & value);

    virtual QColor frameColor() const;

    const QPixmap *pixmap() const;

public Q_SLOTS:
    //! Sets the datasource to \a ds
    inline void setDataSource(const QString &ds) {
        KexiFormDataItemInterface::setDataSource(ds);
    }

    inline void setDataSourcePluginId(const QString &pluginId) {
        KexiFormDataItemInterface::setDataSourcePluginId(pluginId);
    }

    virtual void setText(const QString& text);

    virtual void setPalette(const QPalette &pal);

    virtual void setFrameColor(const QColor& color);

protected Q_SLOTS:
    //! empty
    virtual void setReadOnly(bool readOnly) override;

protected:
    void init();
    void setColumnInfo(KDbConnection *conn, KDbQueryColumnInfo* cinfo) override;
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent* e) override;

    //! Sets value \a value for a widget.
    virtual void setValueInternal(const QVariant& add, bool removeOld) override;

    //! Reimplemented to paint using real frame color instead of froeground.
    //! Also allows to paint more types of frame.
    virtual void drawFrame(QPainter *);

    void updatePixmapLater();

    class Private;
    Private * const d;
};

#endif
