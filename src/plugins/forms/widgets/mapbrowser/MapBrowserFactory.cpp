/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Radosław Wicik <radoslaw@wicik.pl>

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


#include "MapBrowserWidget.h"
#include "MapBrowserFactory.h"

#include <formeditor/WidgetInfo.h>
#include <formeditor/formIO.h>
#include <KoIcon.h>
#include <KLocalizedString>
#include <KDebug>
#include <KLocale>
#include <KPluginFactory>
#include <QVariant>
#include <QVariantList>

MapBrowserFactory::MapBrowserFactory(QObject* parent, const QVariantList& args)
  : KexiDBFactoryBase(parent, "mapbrowser")
{
    Q_UNUSED(args);
    KFormDesigner::WidgetInfo *mapBrowser = new KFormDesigner::WidgetInfo(this);
    mapBrowser->setIconName(koIconName("map_browser"));
    mapBrowser->setClassName("MapBrowserWidget");
    mapBrowser->setName(i18n("Map Browser"));
    mapBrowser->setNamePrefix(i18nc("This string will be used to name widgets of this class. It must _not_ contain white "
                                     "spaces and non latin1 characters.", "mapBrowser"));
    mapBrowser->setDescription(i18n("Displays an interactive map."));
    addClass(mapBrowser);
}

MapBrowserFactory::~MapBrowserFactory()
{

}

QWidget* MapBrowserFactory::createWidget(const QByteArray& classname,
                            QWidget* parent,
                            const char* name,
                            KFormDesigner::Container* container,
                            KFormDesigner::WidgetFactory::CreateWidgetOptions options)
{
    QWidget *w = 0;
    QString text(container->form()->library()->textForWidgetName(name, classname));
//2.0    const bool designMode = options & KFormDesigner::WidgetFactory::DesignViewMode;

    if (classname == "MapBrowserWidget")
        w = new MapBrowserWidget(parent);

    if (w){
        w->setObjectName(name);
        kDebug() << w << w->objectName() << "created";
        return w;
    }
    kWarning() << "w == 0";
    return 0;
}

bool MapBrowserFactory::createMenuActions(const QByteArray &classname, QWidget *w,
                                    QMenu *menu, KFormDesigner::Container *container)
{
    return false;
}

bool MapBrowserFactory::startInlineEditing(InlineEditorCreationArguments& args)
{
    return false;
}

bool MapBrowserFactory::previewWidget(const QByteArray &classname,
                                QWidget *widget, KFormDesigner::Container *)
{
    Q_UNUSED(classname);
    Q_UNUSED(widget);
    return true;
}
     
K_EXPORT_KEXI_FORM_WIDGET_FACTORY_PLUGIN(MapBrowserFactory, mapbrowser)

#include "MapBrowserFactory.moc"

