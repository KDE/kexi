/* This file is part of the KDE project
   Copyright (C) 2006-2012 Jarosław Staniek <staniek@kde.org>

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

#include "kexidbutils.h"
#include <KexiIcon.h>
#include <formeditor/widgetlibrary.h>
#include <kexiutils/utils.h>
#include "kexiformpart.h"
#include "kexiformmanager.h"
#include <widget/utils/kexicontextmenuutils.h>

#include <KIconLoader>
#include <KIconEffect>

#include <KDbQuerySchema>
#include <KDbUtils>
#include <KDb>

#include <QApplication>
#include <QFontMetrics>
#include <QMenu>

class Q_DECL_HIDDEN KexiDBWidgetContextMenuExtender::Private
{
public:
    explicit Private(KexiDataItemInterface* iface_)
      : iface(iface_)
    {
    }

    KexiDataItemInterface* iface;
};


//! Static data for kexi forms
struct KexiFormStatics
{
    QPixmap dataSourceTagIcon() {
        initDataSourceTagIcon();
        return m_dataSourceTagIcon;
    }

    QPixmap dataSourceRTLTagIcon() {
        initDataSourceTagIcon();
        return m_dataSourceRTLTagIcon;
    }

    void initDataSourceTagIcon() {
        if (!m_dataSourceTagIcon.isNull())
            return;
        QFontMetrics fm(QApplication::fontMetrics());
        int size = KIconLoader::global()->currentSize(KIconLoader::Small);
        if (size < KIconLoader::SizeSmallMedium && fm.height() >= KIconLoader::SizeSmallMedium)
            size = KIconLoader::SizeSmallMedium;
        m_dataSourceTagIcon = KexiIcon("data-source-tag").pixmap(size);
        KIconEffect::semiTransparent(m_dataSourceTagIcon);
        m_dataSourceRTLTagIcon = QPixmap::fromImage(m_dataSourceTagIcon.toImage().mirrored(true /*h*/, false /*v*/));
    }
private:
    QPixmap m_dataSourceTagIcon;
    QPixmap m_dataSourceRTLTagIcon;
};

Q_GLOBAL_STATIC(KexiFormStatics, g_KexiFormStatics)

//-------

QColor KexiFormUtils::lighterGrayBackgroundColor(const QPalette& palette)
{
    return KexiUtils::blendedColors(
        palette.color(QPalette::Active, QPalette::Background),
        palette.color(QPalette::Active, QPalette::Base),
        1, 2);
}

QPixmap KexiFormUtils::dataSourceTagIcon()
{
    return g_KexiFormStatics->dataSourceTagIcon();
}

QPixmap KexiFormUtils::dataSourceRTLTagIcon()
{
    return g_KexiFormStatics->dataSourceRTLTagIcon();
}

//-------

KexiDBWidgetContextMenuExtender::KexiDBWidgetContextMenuExtender(QObject* parent, KexiDataItemInterface* iface)
        : QObject(parent)
        , d(new Private(iface))
{
}

KexiDBWidgetContextMenuExtender::~KexiDBWidgetContextMenuExtender()
{
    delete d;
}

void KexiDBWidgetContextMenuExtender::exec(QMenu *menu, const QPoint &globalPos)
{
    updateActions(menu);
    menu->exec(globalPos);
}

void KexiDBWidgetContextMenuExtender::updateActions(QMenu *menu)
{
    if (!menu)
        return;

    // title
    QString icon;
    const QWidget *thisWidget = dynamic_cast<QWidget*>(d->iface);
    if (thisWidget) {
        icon = KexiFormManager::self()->library()->iconName(
                   thisWidget->metaObject()->className());
    }
    if (d->iface->columnInfo()) {
        KexiContextMenuUtils::updateTitle(
            menu, d->iface->columnInfo()->captionOrAliasOrName(),
            KDb::simplifiedFieldTypeName(d->iface->columnInfo()->field()->type()), icon);
    }

    // actions
    const bool readOnly = d->iface->isReadOnly();
    foreach(QAction* action, menu->actions()) {
        const QString text(action->text());
        if (text.startsWith(QObject::tr("Cu&t")) /*do not use i18n()!*/
            || text.startsWith(QObject::tr("C&lear"))
            || text.startsWith(QObject::tr("&Paste"))
            || text.startsWith(QObject::tr("Delete")))
        {
            action->setEnabled(!readOnly);
        }
        else if (text.startsWith(QObject::tr("&Redo")))
        {
//! @todo maybe redo will be enabled one day?
            action->setVisible(false);
        }
    }
}

//------------------

KexiSubwidgetInterface::KexiSubwidgetInterface()
{
}

KexiSubwidgetInterface::~KexiSubwidgetInterface()
{
}
