/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#ifndef RICHTEXTEDIT_DIALOG_H
#define RICHTEXTEDIT_DIALOG_H

#include "kformdesigner_export.h"

#include <QDialog>

class QTextCharFormat;

namespace KFormDesigner
{

//! A simple dialog to edit rich text
/*! It allows to change font name, style and color, alignment. */
class KFORMDESIGNER_EXPORT RichTextDialog : public QDialog
{
    Q_OBJECT

public:
    RichTextDialog(QWidget *parent, const QString &text);
    ~RichTextDialog();

    QString text() const;

public Q_SLOTS:
    void changeFont(const QString &);
    void changeColor(const QColor&);
    void slotActionTriggered(QAction* action);
    void slotCurrentCharFormatChanged(const QTextCharFormat& f);

private:
    class Private;
    Private* const d;
};

}

#endif
