/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIGLOBALVIEWMODESELECTOR_H
#define KEXIGLOBALVIEWMODESELECTOR_H

#include <KexiMainWindowIface.h>
#include <KexiListView.h>

#include <QScopedPointer>

//! A global view mode selector widget
class KexiGlobalViewModeSelector : public KexiListView
{
    Q_OBJECT
public:
    explicit KexiGlobalViewModeSelector(QWidget *parent = 0);
    virtual ~KexiGlobalViewModeSelector();

    //! @return current mode
    Kexi::GlobalViewMode currentMode() const;

    //! Sets current mode
    void setCurrentMode(Kexi::GlobalViewMode mode);

    //! Set arrow color, it depends on color of sibling area so needs to be controlled
    void setArrowColor(const QColor &color);

    //! @return keyboard modifiers (CTRL, SHIFT, etc.) pressed while item was recently clicked.
    //! Use with currentModeChanged();
    //! Useful for handling "Edit text" mode as CTRL+Design.
    Qt::KeyboardModifiers keyboardModifiers() const;

Q_SIGNALS:
    void currentModeChanged(Kexi::GlobalViewMode previousMode);

protected Q_SLOTS:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

protected:
    void paintEvent(QPaintEvent *event) override;

    //! Reimplemented to handle keyboardModifiers() and disable modifiers for selections
    void mousePressEvent(QMouseEvent *event) override;

    //! Emptied to disable double clicks
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    //! Emptied to disable dragging over items that changes selection
    void mouseMoveEvent(QMouseEvent *event) override;

    class Private;
    const QScopedPointer<Private> d;
};

#endif // KEXIGLOBALVIEWMODESELECTOR_H
