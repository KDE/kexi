/* This file is part of the KDE project
   Copyright (C) 2012 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIWELCOMESTATUSBAR_H
#define KEXIWELCOMESTATUSBAR_H

#include <QWidget>

class QUrl;

//! Status sidebar for the welcome page
class KexiWelcomeStatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit KexiWelcomeStatusBar(QWidget* parent = 0);
    ~KexiWelcomeStatusBar();

    QPixmap userProgressPixmap();
    QPixmap externalLinkPixmap();

private slots:
    void showContributionHelp();
    void showShareUsageInfo();
    void showContributionDetails();
    void showDonation();
    void slotShareFeedback();
    void slotCancelled();
    //! Used for async show for speeding up the message displaying
    void slotShowContributionHelpContents();
    void slotMessageWidgetClosed();
    void slotShareContributionDetailsToggled(bool on);
    void slotShareContributionDetailsGroupToggled(bool on);
    void slotToggleContributionDetailsDataVisibility();

private:
    void init();
    void updateContributionGroupCheckboxes();

    class Private;
    Private * const d;
};

#endif
