/***************************************************************************
 * variant.cpp
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

#include "variant.h"

using namespace Kross::Api;

Variant::Variant(const QVariant& value, const QString& name)
    : Value<Variant, QVariant>(value, name)
{
}

Variant::~Variant()
{
}

const QString Variant::getClassName() const
{
    return "Kross::Api::Variant";
}

const QString Variant::getDescription() const
{
    return i18n("Object to handle QVariant values.");
}

const QVariant& Variant::toVariant(Object::Ptr object)
{
    Kross::Api::Variant* variant = (Kross::Api::Variant*)object.data();
    if(! variant)
        throw TypeException("Kross::Api::Variant expected.");
    return variant->getValue();
}

const QString Variant::toString(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    if(variant.type() != QVariant::String &&
       variant.type() != QVariant::CString)
        throw TypeException(QString("Kross::Api::Variant::String expected, but got %1.").arg(variant.typeName()).latin1());
    return variant.toString();
}

uint Variant::toUInt(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    //TODO check for QVariant::UInt ?!
    bool ok;
    uint i = variant.toUInt(&ok);
    if(! ok)
        throw TypeException(QString("Kross::Api::Variant::UInt expected, but got %1.").arg(variant.typeName()).latin1());
    return i;
}

Q_LLONG Variant::toLLONG(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    bool ok;
    Q_LLONG l = variant.toLongLong(&ok);
    if(! ok)
        throw TypeException(QString("Kross::Api::Variant::LLONG expected, but got %1.").arg(variant.typeName()).latin1());
    return l;
}

Q_ULLONG Variant::toULLONG(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    bool ok;
    Q_ULLONG l = variant.toULongLong(&ok);
    if(! ok)
        throw TypeException(QString("Kross::Api::Variant::ULLONG expected, but got %1.").arg(variant.typeName()).latin1());
    return l;
}

bool Variant::toBool(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    if(variant.type() != QVariant::Bool &&
       variant.type() != QVariant::LongLong &&
       variant.type() != QVariant::ULongLong &&
       variant.type() != QVariant::Int &&
       variant.type() != QVariant::UInt
    )
        throw TypeException(QString("Kross::Api::Variant::Bool expected, but got %1.").arg(variant.typeName()).latin1());
    return variant.toBool();
}

QValueList<QVariant> Variant::toList(Object::Ptr object)
{
    QVariant variant = toVariant(object);
    if(variant.type() != QVariant::List)
        throw TypeException(QString("Kross::Api::Variant::List expected, but got %1.").arg(variant.typeName()).latin1());
    return variant.toList();
}
