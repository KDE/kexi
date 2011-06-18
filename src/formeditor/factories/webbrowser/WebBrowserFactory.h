/*
  <The basic code for the web widget in Kexi forms>
    Copyright (C) 2011  Shreya Pandit <shreya@shreyapandit.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef WEBBROWSERFACTORY_H
#define WEBBROWSERFACTORY_H

#include "widgetfactory.h"
#include "container.h"
#include "FormWidgetInterface.h"

class WebBrowserFactory: public KFormDesigner::WidgetFactory
{
  Q_OBJECT
public:
    WebBrowserFactory(QObject* parent, const QVariantList &args);
    virtual ~WebBrowserFactory();
    virtual QWidget* createWidget(const QByteArray &classname, QWidget *parent, const char *name,
                                  KFormDesigner::Container *container,
                                  CreateWidgetOptions options = DefaultOptions);
    virtual bool createMenuActions(const QByteArray &classname, QWidget *w,
                                   QMenu *menu, KFormDesigner::Container *container);
    virtual bool startInlineEditing(InlineEditorCreationArguments& args);
    virtual bool previewWidget(const QByteArray &classname, QWidget *widget,
                               KFormDesigner::Container *container);
};

#endif // WEBBROWSERFACTORY_H
