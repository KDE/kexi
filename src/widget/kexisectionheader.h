/* This file is part of the KDE project
   Copyright (C) 2004-2007 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXISECTIONHEADER_H
#define KEXISECTIONHEADER_H

#include "kexiextwidgets_export.h"

#include <QWidget>

class QEvent;
class QIcon;

//! @short A section header widget acting as a splitter with caption and buttons
/*! see KexiQueryDesignerGuiEditor for example use. */
class KEXIEXTWIDGETS_EXPORT KexiSectionHeader : public QWidget
{
    Q_OBJECT
public:
    KexiSectionHeader(const QString &caption, Qt::Orientation o,
                      QWidget* parent = nullptr);

    virtual ~KexiSectionHeader();

    //! Sets child widget wisible below (or on the right hand) of the section header.
    //! A widget previously set is removed from the layout.
    void setWidget(QWidget * widget);

    void addButton(const QIcon& icon, const QString& toolTip,
                   const QObject * receiver, const char * member);

    virtual QSize sizeHint() const override;

    void setCaption(const QString& caption);
    QString caption() const;

public Q_SLOTS:
    void slotFocus(bool in);

protected:
    virtual bool eventFilter(QObject *o, QEvent *e) override;

private:
    class Private;
    Private * const d;
};

#endif

