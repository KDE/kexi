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

#include "KexiObjectViewTabWidget.h"
#include <KexiIcon.h>
#include <KexiTester.h>
#include <KexiWindow.h>

#include <KLocalizedString>

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QTabBar>
#include <QVBoxLayout>

//! @internal window container created to speedup opening new tabs
class KexiWindowContainer : public QWidget
{
    Q_OBJECT
public:
    explicit KexiWindowContainer(QWidget* parent);

    virtual ~KexiWindowContainer();

    void setWindow(KexiWindow* w);

    KexiWindow* window() { return m_window; }

private:
    QPointer<KexiWindow> m_window;
    QVBoxLayout *lyr;
};

KexiWindowContainer::KexiWindowContainer(QWidget* parent)
    : QWidget(parent)
    , lyr(new QVBoxLayout(this))
{
    lyr->setContentsMargins(0, 0, 0, 0);
}

KexiWindowContainer::~KexiWindowContainer()
{
    //! @todo warning if saveSettings() failed?
    if (m_window) {
        m_window->saveSettings();
        delete (KexiWindow*)m_window;
    }
}

void KexiWindowContainer::setWindow(KexiWindow* w)
{
    m_window = w;
    if (w) {
        lyr->addWidget(w);
    }
}

//-------------------------------------------------

KexiObjectViewTabWidget::KexiObjectViewTabWidget(QWidget *parent, KexiObjectViewWidget* mainWidget)
        : QTabWidget(parent)
        , m_mainWidget(mainWidget)
        , m_tabIndex(-1)
{
    m_closeAction = new QAction(koIcon("tab-close"), xi18n("&Close Tab"), this);
    m_closeAction->setToolTip(xi18n("Close the current tab"));
    m_closeAction->setWhatsThis(xi18n("Closes the current tab."));
    m_closeAllTabsAction = new QAction(xi18n("Cl&ose All Tabs"), this);
    m_closeAllTabsAction->setToolTip(xi18n("Close all tabs"));
    m_closeAllTabsAction->setWhatsThis(xi18n("Closes all tabs."));
    connect(m_closeAction, &QAction::triggered, this, &KexiObjectViewTabWidget::tabCloseRequested);
    connect(m_closeAllTabsAction, &QAction::triggered, this, &KexiObjectViewTabWidget::allTabsCloseRequested);

//! @todo  insert window list in the corner widget as in firefox
#if 0
    // close-tab button:
    QToolButton* rightWidget = new QToolButton(this);
    rightWidget->setDefaultAction(m_closeAction);
    rightWidget->setText(QString());
    rightWidget->setAutoRaise(true);
    rightWidget->adjustSize();
    setCornerWidget(rightWidget, Qt::TopRightCorner);
#endif
    setMovable(true);
    setDocumentMode(true);
    tabBar()->setExpanding(true);
}

KexiObjectViewTabWidget::~KexiObjectViewTabWidget()
{
}

void KexiObjectViewTabWidget::paintEvent(QPaintEvent * event)
{
    if (count() > 0)
        QTabWidget::paintEvent(event);
    else
        QWidget::paintEvent(event);
}

void KexiObjectViewTabWidget::mousePressEvent(QMouseEvent *event)
{
    //! @todo KEXI3 test KexiMainWindowTabWidget's contextMenu event port from KTabWidget
    if (event->button() == Qt::RightButton) {
        int tab = tabBar()->tabAt(event->pos());
        const QPoint realPos(tabBar()->mapToGlobal(event->pos()));
        if (QRect(tabBar()->mapToGlobal(QPoint(0,0)),
              tabBar()->mapToGlobal(QPoint(tabBar()->width()-1, tabBar()->height()-1))).contains(realPos))
        {
            showContextMenuForTab(tab, tabBar()->mapToGlobal(event->pos()));
            return;
        }
    }
    QTabWidget::mousePressEvent(event);
}

KexiWindow* KexiObjectViewTabWidget::window(int index)
{
    KexiWindowContainer *windowContainer = qobject_cast<KexiWindowContainer*>(widget(index));
    return windowContainer ? windowContainer->window() : 0;
}

void KexiObjectViewTabWidget::closeCurrentTab()
{
    emit tabCloseRequested(m_tabIndex);
}

void KexiObjectViewTabWidget::closeAllTabs()
{
    emit allTabsCloseRequested();
}

int KexiObjectViewTabWidget::addEmptyContainerTab(const QIcon &icon, const QString &label)
{
    KexiWindowContainer *windowContainer = new KexiWindowContainer(this);
    return addTab(windowContainer, icon, label);
}

void KexiObjectViewTabWidget::setWindowForTab(int index, KexiWindow *window)
{
    KexiWindowContainer *windowContainer = qobject_cast<KexiWindowContainer*>(widget(index));
    if (windowContainer) {
        if (windowContainer->window()) {
            qWarning() << "tab" << index << "already has KexiWindow assigned";
            return;
        }
        windowContainer->setWindow(window);
    }
}

void KexiObjectViewTabWidget::showContextMenuForTab(int index, const QPoint& point)
{
    QMenu menu;
    if (index >= 0) {
        menu.addAction(m_closeAction);
    }
    if (count() > 0) {
        menu.addAction(m_closeAllTabsAction);
    }
    //! @todo add "&Detach Tab"
    if (menu.actions().isEmpty()) {
        return;
    }
    setTabIndexFromContextMenu(index);
    menu.exec(point);
}

void KexiObjectViewTabWidget::setTabIndexFromContextMenu(int clickedIndex)
{
    if (currentIndex() == -1) {
        m_tabIndex = -1;
        return;
    }
    m_tabIndex = clickedIndex;
}

#include "KexiObjectViewTabWidget.moc"
