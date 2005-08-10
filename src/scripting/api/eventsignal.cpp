/***************************************************************************
 * eventsignal.cpp
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

#include "eventsignal.h"

#include "variant.h"
#include "qtobject.h"

#include <qmetaobject.h>
#include <private/qucom_p.h> // for the Qt QUObject API.

using namespace Kross::Api;

EventSignal::EventSignal(const QString& name, Object::Ptr parent, QObject* sender, QCString signal)
    : Event<EventSignal>(name, parent)
    , m_sender(sender)
    , m_signal(signal) //QObject::normalizeSignalSlot(signal)
{
}

EventSignal::~EventSignal()
{
}

const QString EventSignal::getClassName() const
{
    return "Kross::Api::EventSignal";
}

Object::Ptr EventSignal::call(const QString& name, KSharedPtr<List> arguments)
{
#ifdef KROSS_API_EVENTSIGNAL_CALL_DEBUG
    kdDebug() << QString("EventSignal::call() name=%1 m_signal=%2 arguments=%3").arg(name).arg(m_signal).arg(arguments->toString()) << endl;
#endif

    QString n = m_signal;

    if(! n.startsWith("2"))
        throw new Exception(QString("Invalid signal '%1'.").arg(n));
    //if(n.startsWith("2")) n.remove(0,1);
    n.remove(0,1);

    int signalid = m_sender->metaObject()->findSignal(n.latin1(), false);
    if(signalid < 0)
        throw new Exception(QString("No such signal '%1'.").arg(n));

    QUObject* uo = QtObject::toQUObject(n, arguments);
    m_sender->qt_emit(signalid, uo); // emit the signal
    delete [] uo;

    return new Variant(true, "Kross::Api::EventSignal::Bool");
}
