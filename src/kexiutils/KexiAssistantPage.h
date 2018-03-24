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

#ifndef KEXIASSISTANTPAGE_H
#define KEXIASSISTANTPAGE_H

#include <QWidget>

#include "kexiutils_export.h"

class KexiLinkWidget;

//! A single page for assistant (KexiAssistantWidget).
class KEXIUTILS_EXPORT KexiAssistantPage : public QWidget
{
    Q_OBJECT
public:
    KexiAssistantPage(const QString& title, const QString& description,
                      QWidget* parent = 0);
    virtual ~KexiAssistantPage();
    void setContents(QWidget* widget);
    void setContents(QLayout* layout);

    /**
     * Returns recent focus widget
     *
     * Useful to maintain focus when activation returns to the page
     */
    QWidget* recentFocusWidget() const;

    /**
     * Sets recent focus widget
     *
     * Useful to maintain focus when activation returns to the page
     */
    void setRecentFocusWidget(QWidget* widget);

    /**
     * Restores focus on recent focus widget
     *
     * If the widget is a QLineEdit, text selection and cursor position remembered during the
     * setRecentFocusWidget() call is also restored. If the widget is not a QLineEdit, only focus is
     * restored. This method does nothing if there is no focus widget set or the widget has been
     * deleted in the meantime.
     *
     * @see recentFocusWidget
     */
    void focusRecentFocusWidget();

    KexiLinkWidget* backButton();
    KexiLinkWidget* nextButton();
    QString title() const;
    QString description() const;
public Q_SLOTS:
    void setDescription(const QString& text);
    void setBackButtonVisible(bool set);
    void setNextButtonVisible(bool set);
    //! Moves to previous page; if not, warning is displayed on debug output
    void back();
    //! Moves to previous page if possible; if not, nothing happens
    void tryBack();
    //! Moves to next page
    void next();
Q_SIGNALS:
    void backRequested(KexiAssistantPage* page);
    void tryBackRequested(KexiAssistantPage* page);
    void nextRequested(KexiAssistantPage* page);
    void cancelledRequested(KexiAssistantPage* page);

private Q_SLOTS:
    void slotLinkActivated(const QString& link);
    void slotCancel();

private:
    class Private;
    Private * const d;
};

#endif
