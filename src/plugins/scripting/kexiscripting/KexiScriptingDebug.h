/* This file is part of the KDE project
   Copyright (C) 2017 Adam Pigg <adam@piggz.co.uk>

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

#ifndef KEXISCRIPTINGDEBUG_H
#define KEXISCRIPTINGDEBUG_H
#include <QDebug>


#ifdef KEXI_SCRIPTING_DEBUG
#define KexiScriptingDebug qDebug
#define KexiScriptingWarning qWarning
#else
#define KexiScriptingDebug while (0) qDebug
#define KexiScriptingWarning while (0) qWarning
#endif

#endif
