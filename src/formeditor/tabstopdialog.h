/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005,2010 Jarosław Staniek <staniek@kde.org>

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

#ifndef TABSTOPEDIT_DIALOG_H
#define TABSTOPEDIT_DIALOG_H

#include "kformdesigner_export.h"

#include <QDialog>

class QTreeWidgetItem;

namespace KFormDesigner
{

class Form;

//! A dialog to edit Form tab stops
/*! The user can change the order by dragging list items or using buttons at the right.
  The tab stops can be arranged automatically (see \ref Form::autoAssignTabStops()). */
class KFORMEDITOR_EXPORT TabStopDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TabStopDialog(QWidget *parent);
    virtual ~TabStopDialog();

    bool autoTabStops() const;

public Q_SLOTS:
    int exec(KFormDesigner::Form *form);

protected Q_SLOTS:
    void moveItemUp();
    void moveItemDown();
    void updateButtons(QTreeWidgetItem *item);
    void slotRadioClicked(bool isOn);
    void slotSelectionChanged();

protected:

    class Private;
    Private* const d;
};

}

#endif
