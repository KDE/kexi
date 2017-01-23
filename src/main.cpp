/* This file is part of the KDE project

   begin : Sun Jun  9 12:15:11 CEST 2002

   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <QApplication>
#include <main/KexiMainWindow.h>

int main(int argc, char *argv[])
{
    //! @todo use non-GUI app when needed
    QApplication app(argc, argv);
    const int result = KexiMainWindow::create(QCoreApplication::arguments());
    if (result != 0) {
        return result;
    }
    return app.exec();
}
