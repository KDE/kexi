/* This file is part of the KDE project
   Copyright (C) 2011-2018 Jarosław Staniek <staniek@kde.org>

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

#include "KexiAssistantWidget.h"
#include "KexiAssistantPage.h"
#include "KexiAnimatedLayout.h"
#include <kexiutils/utils.h>

#include <QDebug>
#include <QStyle>
#include <QStack>
#include <QPointer>

class Q_DECL_HIDDEN KexiAssistantWidget::Private
{
public:
    explicit Private(KexiAssistantWidget *qq)
        : q(qq)
    {
    }

    ~Private()
    {
    }

    void addPage(KexiAssistantPage* page) {
        lyr->addWidget(page);
        connect(page, &KexiAssistantPage::backRequested, q, &KexiAssistantWidget::previousPageRequested);
        connect(page, &KexiAssistantPage::tryBackRequested, q, &KexiAssistantWidget::tryPreviousPageRequested);
        connect(page, &KexiAssistantPage::nextRequested, q, &KexiAssistantWidget::nextPageRequested);
        connect(page, &KexiAssistantPage::cancelledRequested, q, &KexiAssistantWidget::cancelRequested);
    }

    KexiAnimatedLayout *lyr;
    QStack< QPointer<KexiAssistantPage> > stack;

private:
    KexiAssistantWidget* q;
};

// ----

KexiAssistantWidget::KexiAssistantWidget(QWidget* parent)
 : QWidget(parent)
 , d(new Private(this))
{
    QVBoxLayout *mainLyr = new QVBoxLayout(this);
    d->lyr = new KexiAnimatedLayout;
    mainLyr->addLayout(d->lyr);
    int margin = style()->pixelMetric(QStyle::PM_MenuPanelWidth, 0, 0)
        + KexiUtils::marginHint();
    mainLyr->setContentsMargins(margin, margin, margin, margin);
}

KexiAssistantWidget::~KexiAssistantWidget()
{
    delete d;
}

void KexiAssistantWidget::addPage(KexiAssistantPage* page)
{
    d->addPage(page);
}

void KexiAssistantWidget::previousPageRequested(KexiAssistantPage* page)
{
    if (d->stack.count() < 2) {
        qWarning() << "Page stack's' count < 2";
        return;
    }
    tryPreviousPageRequested(page);
}

void KexiAssistantWidget::tryPreviousPageRequested(KexiAssistantPage* page)
{
    Q_UNUSED(page);
    if (d->stack.count() < 2) {
        return;
    }
    d->stack.pop();
    setCurrentPage(d->stack.top());
}

void KexiAssistantWidget::nextPageRequested(KexiAssistantPage* page)
{
    Q_UNUSED(page);
}

void KexiAssistantWidget::cancelRequested(KexiAssistantPage* page)
{
    Q_UNUSED(page);
}

KexiAssistantPage* KexiAssistantWidget::currentPage() const
{
    return dynamic_cast<KexiAssistantPage*>(d->lyr->currentWidget());
}

void KexiAssistantWidget::setCurrentPage(KexiAssistantPage* page)
{
    if (!page) {
        qWarning() << "!page";
        return;
    }
    if (currentPage() && currentPage() != page) {
        QWidget *actualFocusWidget = currentPage()->focusWidget();
        if (actualFocusWidget) {
            currentPage()->setRecentFocusWidget(actualFocusWidget);
        }
    }
    d->lyr->setCurrentWidget(page);
    if (page->recentFocusWidget()) {
        page->focusRecentFocusWidget();
    } else {
        qWarning() << "Missing recent focus widget for" << page
                   << "-- this cat lead to usability isses; please call "
                      "KexiAssistantPage::setRecentFocusWidget() for that page.";
    }
    if (d->stack.isEmpty() || d->stack.top() != page) {
        int index = d->stack.indexOf(page);
        if (index != -1) {
            d->stack.remove(index);
        }
        d->stack.push(page);
    }
}

