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

#ifndef KEXIOBJECTVIEWWIDGET_H
#define KEXIOBJECTVIEWWIDGET_H

#include <QWidget>

class KexiObjectViewTabWidget;
class KexiProjectNavigator;
class KexiPropertyPaneWidget;
class KexiWindow;

//! @short A widget for object view, used in edit and design global view mode
/*! Contents:
 @verbatim
 |---|--------------|-------------|---------------|---|
 |   |              |     tabs    |               |   |
 |tab|              |-------------|               |tab|
 |bar|Prj. navigator|Object's view|Property editor|bar|
 |   |  pane        |             |   pane        |   |
 |---|--------------|-------------|---------------|---|
 @endverbatim
*/
class KexiObjectViewWidget : public QWidget
{
    Q_OBJECT
public:
    enum Flag {
        NoFlags = 0,
        ProjectNavigatorEnabled = 1,
        DefaultFlags = ProjectNavigatorEnabled
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    explicit KexiObjectViewWidget(Flags flags = DefaultFlags);

    virtual ~KexiObjectViewWidget();

    KexiProjectNavigator* projectNavigator() const;
    KexiObjectViewTabWidget* tabWidget() const;
    KexiPropertyPaneWidget* propertyPane() const;

    void setProjectNavigatorVisible(bool set);
    void setPropertyPaneVisible(bool set);

    void setSidebarWidths(int projectNavigatorWidth, int propertyEditorWidth);
    void getSidebarWidths(int *projectNavigatorWidth, int *propertyEditorWidth) const;
    void updateSidebarWidths();

protected Q_SLOTS:
    void slotCurrentTabIndexChanged(int index);
    void slotSplitterMoved(int pos, int index);

Q_SIGNALS:
    void currentTabIndexChanged(int index);
    void activeWindowChanged(KexiWindow *window, KexiWindow *prevWindow);
    void closeWindowRequested(int index);
    void closeAllWindowsRequested();
    void projectNavigatorAnimationFinished(bool visible);

protected:
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *e) Q_DECL_OVERRIDE;

private:
    void setupCentralWidget();

    class Private;
    const QScopedPointer<Private> d;

    friend class KexiObjectViewTabWidget;
};

#endif
