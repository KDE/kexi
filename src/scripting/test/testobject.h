/***************************************************************************
 * testobject.h
 * This file is part of the KDE project
 * copyright (C)2004-2005 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef KROSS_TEST_TESTOBJECT_H
#define KROSS_TEST_TESTOBJECT_H

#include <qobject.h>
#include <qstring.h>

class TestObject : public QObject
{
        Q_OBJECT

        //Q_PROPERTY(QString testProperty READ testProperty WRITE setTestProperty)

    public:
        TestObject(QObject* parent);
        ~TestObject();

        //QString m_prop;
        //QString testProperty() const { return m_prop; }
        //void setTestProperty(QString prop) { m_prop = prop; }

    signals:
        void testSignal();
        void testSignalString(const QString&);
    public slots:
        void testSlot();
        void testSlot2();
    private slots:
        void testSignalSlot();
};

#endif
