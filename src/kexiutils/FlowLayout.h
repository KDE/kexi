/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2007-2012 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFLOWLAYOUT_H
#define KEXIFLOWLAYOUT_H

#include <QLayout>
#include <QList>
#include "kexiutils_export.h"

//! @short a special "flow" layout
//! @todo Vertical KexiFlowLayout ported to Qt4 but not tested.
class KEXIUTILS_EXPORT KexiFlowLayout : public QLayout
{
public:
    explicit KexiFlowLayout(QWidget *parent, int margin = 0, int spacing = -1);
    explicit KexiFlowLayout(QLayout* parent, int margin = 0, int spacing = -1);
    explicit KexiFlowLayout(int margin = 0, int spacing = -1);

    virtual ~KexiFlowLayout();

    /*! \return the widgets in the order of the layout,
     ie as it is stored in m_list. You must delete the list after using it. */
    QList<QWidget*>* widgetList() const;

    /*! Sets layout's orientation to \a orientation. Default orientation is Vertical. */
    void  setOrientation(Qt::Orientation orientation);


    /*! \return layout's orientation. */
    Qt::Orientation orientation() const;

    void setJustified(bool justify);
    bool isJustified() const;

    virtual void addItem(QLayoutItem *item);
    virtual void addSpacing(int size);
    void insertWidget(int index, QWidget* widget, int stretch = 0, Qt::Alignment alignment = 0);
    virtual void invalidate();

    virtual bool hasHeightForWidth() const;
    virtual int heightForWidth(int width) const;
    virtual QSize sizeHint() const;
    virtual QSize minimumSize() const;
    virtual Qt::Orientations expandingDirections() const;
    virtual int count() const;
    virtual bool isEmpty() const;
    virtual void setGeometry(const QRect&);
    virtual QLayoutItem *itemAt(int index) const;
    virtual QLayoutItem *takeAt(int index);

protected:
    int simulateLayout(const QRect &r);
    int doHorizontalLayout(const QRect&, bool testonly = false);
    int doVerticalLayout(const QRect&, bool testonly = false);

private:
    class Private;

    Private* const d;
};

#endif
