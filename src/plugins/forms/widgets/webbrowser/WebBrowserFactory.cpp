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


#include "WebBrowserFactory.h"
#include <formeditor/WidgetInfo.h>
#include <formeditor/formIO.h>
#include "kexidataawarewidgetinfo.h"
#include "WebBrowserWidget.h"

#include <KexiIcon.h>

#include <QVariant>
#include <QVariantList>

#include <kdebug.h>
#include <klocale.h>

WebBrowserFactory::WebBrowserFactory(QObject* parent, const QVariantList& args)
  : KexiDBFactoryBase(parent, "webbrowser")
{
    Q_UNUSED(args);
    KexiDataAwareWidgetInfo* webBrowser = new KexiDataAwareWidgetInfo(this);
    webBrowser->setIconName(koIconName("web_browser"));
    webBrowser->setClassName("WebBrowserWidget");
    webBrowser->setName(i18n("Web Browser"));
    webBrowser->setNamePrefix(
        i18nc("A prefix for identifiers of web browser widgets. Based on that, identifiers such as "
            "webBrowser1, webBrowser2 are generated. "
            "This string can be used to refer the widget object as variables in programming "
            "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
            "should start with lower case letter and if there are subsequent words, these should "
            "start with upper case letter. Example: smallCamelCase. "
            "Moreover, try to make this prefix as short as possible.",
            "webBrowser"));
    webBrowser->setDescription(i18n("Web widget with browsing features."));
    webBrowser->setInlineEditingEnabledWhenDataSourceSet(false);
    addClass(webBrowser);

    setPropertyDescription("zoomFactor", i18n("Zoom Factor"));
    setPropertyDescription("url", i18n("Url"));
}

WebBrowserFactory::~WebBrowserFactory()
{

}

QWidget* WebBrowserFactory::createWidget(const QByteArray& classname,
                            QWidget* parent,
                            const char* name,
                            KFormDesigner::Container* container,
                            KFormDesigner::WidgetFactory::CreateWidgetOptions options)
{
    Q_UNUSED(options);
    QWidget *w = 0;
    QString text(container->form()->library()->textForWidgetName(name, classname));

    if (classname == "WebBrowserWidget")
        w = new WebBrowserWidget(parent);

    if (w){
        w->setObjectName(name);
        kDebug() << w << w->objectName() << "created";
        return w;
    }
    kWarning() << "w == 0";
    return 0;
}

bool WebBrowserFactory::createMenuActions(const QByteArray &classname, QWidget *w,
                                    QMenu *menu, KFormDesigner::Container *container)
{
    Q_UNUSED(classname);
    Q_UNUSED(w);
    Q_UNUSED(menu);
    Q_UNUSED(container);
    return false;
}

bool WebBrowserFactory::startInlineEditing(InlineEditorCreationArguments& args)
{
    Q_UNUSED(args);
    return false;
}

bool WebBrowserFactory::previewWidget(const QByteArray &classname,
                                QWidget *widget, KFormDesigner::Container *)
{
    Q_UNUSED(classname);
    Q_UNUSED(widget);
    return true;
}
     
K_EXPORT_KEXIFORMWIDGETS_PLUGIN(WebBrowserFactory, webbrowser)

#include "WebBrowserFactory.moc"
