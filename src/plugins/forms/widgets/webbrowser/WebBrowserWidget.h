/* This file is part of the KDE project
   Copyright (C) 2011  Shreya Pandit <shreya@shreyapandit.com>

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

#ifndef WEBBROWSERWIDGET_H
#define WEBBROWSERWIDGET_H
#include <QProgressBar>
#include <QWidget>
#include "widgetfactory.h"
#include "container.h"
#include <formeditor/FormWidgetInterface.h>
#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include<QWebView>

class KPushButton;

class QWebView;
class QVBoxLayout;
class QWebHistory;
class QHBoxLayout;
class QProgressBar;
class QHBoxLayout;
class QUrl;

class WebBrowserWidget : public QWidget,
                         public KexiFormDataItemInterface,
                         public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePartClass WRITE setDataSourcePartClass)
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QIcon icon READ icon)
    Q_PROPERTY(bool modified READ modified)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale)
     
public:
    explicit WebBrowserWidget(QWidget *parent = 0);
    ~WebBrowserWidget();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePartClass() const {
        return KexiFormDataItemInterface::dataSourcePartClass();
    }

    inline QString url() const {

        return m_view->url().toString();
    }
    
    inline bool modified() const {

         return m_view->isModified();
    }

    inline QString title() const {

         return m_view->title();
    }

    inline qreal zoomFactor() const {

        return m_view->zoomFactor();
    }
  
    inline QIcon icon() const 
    {
        return m_view->icon();
    }
    
    inline qreal textScale () const
    {
      return m_view->textSizeMultiplier();
    }  
    
    virtual QVariant value();
    virtual bool valueIsNull();
    virtual bool valueIsEmpty();
    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();
    virtual void clear();
    void updateToolBar();
    bool isReadOnly() const;
    virtual void setReadOnly(bool readOnly);  
    virtual void setInvalidState(const QString& displayText);

public slots:
    void setDataSource(const QString &ds);
    void setDataSourcePartClass(const QString &ds);
    void setUrl(const QString& url);
    void setZoomFactor(qreal factor);
    void setTextScale(qreal scale);
    void hide_bar();
    
protected:
    virtual void setValueInternal(const QVariant& add, bool removeOld); 
    void setUrl(const QUrl& url);
    bool m_readOnly;

private:
    QWebView* m_view;
    QVBoxLayout* v_layout;
    QWebHistory* m_history;
    QProgressBar* m_pbar;
    bool  m_urlChanged_enabled;
    KPushButton* m_back;
    KPushButton* m_forward;
    KPushButton* m_reload;  
    KPushButton* m_stop;  
    QHBoxLayout* h_layout;
};

#endif 
