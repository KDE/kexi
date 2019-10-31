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

#ifndef KEXIWIDGETWIDTHANIMATOR_H
#define KEXIWIDGETWIDTHANIMATOR_H

#include "kexiextwidgets_export.h"
#include <QObject>

//! A tool that animates showing and hiding a given widget by changing its width.
class KEXIEXTWIDGETS_EXPORT KexiWidgetWidthAnimator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setWidth)
public:
    //! Sets up width animator object for @a targetWidget widget.
    //! It is expected that the widget is a child of a QSplitter.
    explicit KexiWidgetWidthAnimator(QWidget *targetWidget);
    ~KexiWidgetWidthAnimator();
    void setVisible(bool set);

Q_SIGNALS:
    void animationFinished(bool visible);

protected:
    int width() const;
    void setWidth(int width);
    bool eventFilter(QObject *obj, QEvent *event) override;

protected Q_SLOTS:
    void slotWidthAnimationFinished();

private:
    class Private;
    Private* const d;
};

#endif
