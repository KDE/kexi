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

#ifndef KEXIOBJECTVIEWTABWIDGET_H
#define KEXIOBJECTVIEWTABWIDGET_H

#include <QTabWidget>
#include <KDbTristate>

class KexiObjectViewWidget;
class KexiWindow;

//! @internal tab widget acting as central widget for KexiMainWindow
class KexiObjectViewTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    KexiObjectViewTabWidget(QWidget *parent, KexiObjectViewWidget *mainWidget);
    virtual ~KexiObjectViewTabWidget();

    //! @return window for tab @a index
    KexiWindow *window(int index);

public Q_SLOTS:
    void closeCurrentTab();
    void closeAllTabs();

    //! Adds a new tab with empty container widget. @return index of the new tab.
    int addEmptyContainerTab(const QIcon &icon, const QString &label);

    //! Sets window for tab @a index previously created using addEmptyContainerTab()
    void setWindowForTab(int index, KexiWindow *window);

protected:
    //! Shows context menu for tab at @a index at point @a point.
    //! If @a index is -1, context menu for empty area is requested.
    void showContextMenuForTab(int index, const QPoint& point);

    //! Reimplemented to hide frame when no tabs are displayed
    virtual void paintEvent(QPaintEvent * event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    KexiObjectViewWidget *m_mainWidget;

private:
    int m_tabIndex;

    void setTabIndexFromContextMenu(int clickedIndex);
};

#endif
