/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiObjectViewWidget.h"
#include "KexiObjectViewTabWidget.h"
#include "KexiPropertyPaneWidget.h"
#include <KexiPropertyEditorView.h>
#include <KexiWidgetWidthAnimator.h>
#include <KexiProjectNavigator.h>
#include <KexiTester.h>
#include <KexiStyle.h>
#include <KexiWindow.h>
#include <KexiMainWindowIface.h>

#include <KActionCollection>

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QSplitter>


class KexiObjectViewWidget::Private
{
public:
    Private()
     : navigator(0)
     , navigatorWidthAnimator(0)
     , propertyPane(0)
     , propertyPaneWidthAnimator(0)
     , projectNavigatorWidthToSet(-1)
     , propertyPaneWidthToSet(-1)
    {
    }
    KexiProjectNavigator* navigator;
    KexiWidgetWidthAnimator* navigatorWidthAnimator;
    KexiObjectViewTabWidget* tabWidget;
    QPointer<KexiWindow> previouslyActiveWindow;
    KexiPropertyPaneWidget *propertyPane;
    KexiWidgetWidthAnimator* propertyPaneWidthAnimator;
    //QMap<KMultiTabBar::KMultiTabBarPosition, KMultiTabBar*> multiTabBars;
    QSplitter *splitter;
    int projectNavigatorWidthToSet;
    int propertyPaneWidthToSet;
};


KexiObjectViewWidget::KexiObjectViewWidget(Flags flags)
        : QWidget()
        , d(new Private)
{
    QHBoxLayout *mainLyr = new QHBoxLayout;
    mainLyr->setContentsMargins(0, 0, 0, 0);
    mainLyr->setSpacing(0);
    setLayout(mainLyr);

    d->splitter = new QSplitter(Qt::Horizontal);
    connect(d->splitter, &QSplitter::splitterMoved,
            this, &KexiObjectViewWidget::slotSplitterMoved);
    mainLyr->addWidget(d->splitter);

    // Left tab bar
//    KMultiTabBar *mtbar = new KMultiTabBar(KMultiTabBar::Left);
//    mtbar->setStyle(KMultiTabBar::VSNET);
//    mainLyr->addWidget(mtbar);
//    d->multiTabBars.insert(mtbar->position(), mtbar);

    if (flags & ProjectNavigatorEnabled) {
        // Project navigator
        //    KexiDockableWidget* navDockableWidget = new KexiDockableWidget;
        d->navigator = new KexiProjectNavigator(d->splitter);
        kexiTester() << KexiTestObject(d->navigator, "KexiProjectNavigator");
        //navDockableWidget->setWidget(d->navigator);
        KexiStyle::setSidebarsPalette(d->navigator);
        d->navigatorWidthAnimator = new KexiWidgetWidthAnimator(d->navigator);
        connect(d->navigatorWidthAnimator, &KexiWidgetWidthAnimator::animationFinished,
                this, &KexiObjectViewWidget::projectNavigatorAnimationFinished);
    }

    //d->navDockWidget = new KexiDockWidget(d->navigator->windowTitle(), d->objectViewWidget);
    //d->navDockWidget->setObjectName("ProjectNavigatorDockWidget");
    //d->objectViewWidget->addDockWidget(
    //    applyRightToLeftToDockArea(Qt::LeftDockWidgetArea), d->navDockWidget,
    //    Qt::Vertical);
    //navDockableWidget->setParent(d->navDockWidget);
    //d->navDockWidget->setWidget(navDockableWidget);

    // Central tab widget
    d->tabWidget = new KexiObjectViewTabWidget(d->splitter, this);
    d->tabWidget->setTabsClosable(true);
    connect(d->tabWidget, &KexiObjectViewTabWidget::currentChanged,
            this, &KexiObjectViewWidget::slotCurrentTabIndexChanged);
    connect(d->tabWidget, &KexiObjectViewTabWidget::tabCloseRequested,
            this, &KexiObjectViewWidget::closeWindowRequested);

    if (flags & PropertyPaneEnabled) {
        // Property editor
        d->propertyPane = new KexiPropertyPaneWidget(d->splitter);
        KexiStyle::setSidebarsPalette(d->propertyPane);
        KexiStyle::setSidebarsPalette(d->propertyPane->editor());

        d->propertyPaneWidthAnimator = new KexiWidgetWidthAnimator(d->propertyPane);
    }

//    mtbar = new KMultiTabBar(KMultiTabBar::Right);
//    mtbar->setStyle(KMultiTabBar::VSNET);
//    mainLyr->addWidget(mtbar);
//    d->multiTabBars.insert(mtbar->position(), mtbar);
}

KexiObjectViewWidget::~KexiObjectViewWidget()
{
}

KexiProjectNavigator* KexiObjectViewWidget::projectNavigator() const
{
    return d->navigator;
}

KexiObjectViewTabWidget* KexiObjectViewWidget::tabWidget() const
{
    return d->tabWidget;
}

KexiPropertyPaneWidget* KexiObjectViewWidget::propertyPane() const
{
    return d->propertyPane;
}

void KexiObjectViewWidget::slotCurrentTabIndexChanged(int index)
{
    KexiWindow *window = d->tabWidget->window(index);
    if (!window || (KexiWindow*)d->previouslyActiveWindow == window) {
        return;
    }
    emit activeWindowChanged(window, d->previouslyActiveWindow);
    d->previouslyActiveWindow = window;
    emit currentTabIndexChanged(index);
}

void KexiObjectViewWidget::setSidebarWidths(int projectNavigatorWidth, int propertyEditorWidth)
{
    d->projectNavigatorWidthToSet = projectNavigatorWidth;
    d->propertyPaneWidthToSet = propertyEditorWidth;
}

void KexiObjectViewWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    //qDebug() << "___" << e->size() << size() << isVisible();
    if (isVisible()) {
        updateSidebarWidths();
    }
}

void KexiObjectViewWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    updateSidebarWidths();
}

const int PROJECT_NAVIGATOR_INDEX = 0;
const int MAIN_AREA_INDEX = 1;
const int PROPERTY_EDITOR_INDEX = 2;

void KexiObjectViewWidget::updateSidebarWidths()
{
    QList<int> sizes(d->splitter->sizes());
    if (sizes.count() <= 1) {
        return;
    }
    //qDebug() << "updateSidebarWidths" << d->projectNavigatorWidthToSet << d->propertyEditorWidthToSet << sizes << d->splitter->width() << isVisible();
    if (d->projectNavigatorWidthToSet <= 0) {
        if (d->navigator) {
            sizes[PROJECT_NAVIGATOR_INDEX] = d->navigator->sizeHint().width();
        }
    } else {
        sizes[PROJECT_NAVIGATOR_INDEX] = d->projectNavigatorWidthToSet;
    }
    if (sizes.count() >= (PROPERTY_EDITOR_INDEX+1)) {
        if (d->propertyPane && d->propertyPane->isVisible()) {
            if (d->propertyPaneWidthToSet <= 0) {
                d->propertyPaneWidthToSet = d->propertyPane->sizeHint().width();
            }
            sizes[PROPERTY_EDITOR_INDEX] = d->propertyPaneWidthToSet;
            sizes[MAIN_AREA_INDEX] = d->splitter->width() - sizes[PROJECT_NAVIGATOR_INDEX] - sizes[PROPERTY_EDITOR_INDEX];
        } else {
            sizes[PROPERTY_EDITOR_INDEX] = 0;
            sizes[MAIN_AREA_INDEX] = d->splitter->width() - sizes[PROJECT_NAVIGATOR_INDEX];
        }
    }
    d->splitter->setSizes(sizes);
    //qDebug() << "updateSidebarWidths" << sizes << d->splitter->sizes();
}

void KexiObjectViewWidget::getSidebarWidths(int *projectNavigatorWidth, int *propertyEditorWidth) const
{
    QList<int> sizes(d->splitter->sizes());
    if (sizes.count() < (PROPERTY_EDITOR_INDEX+1)) {
        *projectNavigatorWidth = -1;
        *propertyEditorWidth = -1;
        return;
    }

    //qDebug() << "getSidebarWidths" << d->propertyEditorTabWidget->width();
    *projectNavigatorWidth = (sizes.count() >= (PROJECT_NAVIGATOR_INDEX+1) && sizes[PROJECT_NAVIGATOR_INDEX] > 0)
            ? sizes[PROJECT_NAVIGATOR_INDEX] : d->projectNavigatorWidthToSet;
    *propertyEditorWidth = (sizes.count() >= (PROPERTY_EDITOR_INDEX+1) && sizes[PROPERTY_EDITOR_INDEX] > 0)
            ? sizes[PROPERTY_EDITOR_INDEX] : d->propertyPaneWidthToSet;
    //qDebug() << "getSidebarWidths" << *projectNavigatorWidth << *propertyEditorWidth;
}

void KexiObjectViewWidget::slotSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos)
    //qDebug() << "slotSplitterMoved" << pos << index;
    QList<int> sizes(d->splitter->sizes());
    if (index == PROJECT_NAVIGATOR_INDEX + 1) {
        if (sizes.count() >= (PROJECT_NAVIGATOR_INDEX+1)) {
            d->projectNavigatorWidthToSet = sizes[PROJECT_NAVIGATOR_INDEX];
        }
    } else if (index == PROPERTY_EDITOR_INDEX) {
        if (sizes.count() >= (PROPERTY_EDITOR_INDEX+1)) {
            d->propertyPaneWidthToSet = sizes[PROPERTY_EDITOR_INDEX];
        }
    }
}

void KexiObjectViewWidget::setProjectNavigatorVisible(bool set)
{
    QAction *action_show_nav = KexiMainWindowIface::global()->actionCollection()->action("view_navigator");
    action_show_nav->setChecked(set);
    d->navigatorWidthAnimator->setVisible(set);
}

void KexiObjectViewWidget::setPropertyPaneVisible(bool set)
{
    QAction *action_show_propeditor = KexiMainWindowIface::global()->actionCollection()->action("view_propeditor");
    action_show_propeditor->setChecked(set);
    d->propertyPaneWidthAnimator->setVisible(set);
}
