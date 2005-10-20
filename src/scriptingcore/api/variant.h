/***************************************************************************
 * variant.h
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#ifndef KROSS_API_VARIANT_H
#define KROSS_API_VARIANT_H

#include <qstring.h>
#include <qvariant.h>
//#include <kdebug.h>

#include "object.h"
#include "value.h"
#include "exception.h"

namespace Kross { namespace Api {

    /**
     * Variant value to wrap a QVariant into a \a Kross::Api::Value
     * to enable primitive types like strings or numerics.
     */
    class Variant : public Value<Variant, QVariant>
    {
            friend class Value<Variant, QVariant>;
        public:

            /**
             * Constructor.
             *
             * \param value The initial QVariant-value
             *        this Variant-Object has.
             * \param name The name this Value has.
             */
            Variant(const QVariant& value, const QString& name = "variant");

            /**
             * Destructor.
             */
            virtual ~Variant();

            /// \see Kross::Api::Object::getClassName()
            virtual const QString getClassName() const;

            /**
             * \return a string representation of the variant.
             *
             * \see Kross::Api::Object::toString()
             */
            virtual const QString toString();

            /**
             * \return a more detailed classname for the passed \p object
             * variant type.
             *
             * \throw TypeException If the \p object isn't a valid
             *        \a Variant instance.
             * \param object the variant object we should return a more
             *        detailed classname for.
             * \return If as example the passed \p object is a
             *         QVariant::String then "Kross::Api::Variant::String"
             *         will be returned.
             */
            static const QString getVariantType(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a QVariant.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a QVariant converted object.
             */
            static const QVariant& toVariant(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a QString.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a QString converted object.
             */
            static const QString toString(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a uint.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a uint converted object.
             */
            static uint toUInt(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a Q_LLONG.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a Q_LLONG converted object.
             */
            static Q_LLONG toLLONG(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a Q_ULLONG.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a Q_ULLONG converted object.
             */
            static Q_ULLONG toULLONG(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a boolean value.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a bool converted object.
             */
            static bool toBool(Object::Ptr object);

            /**
             * Try to convert the given \a Object into
             * a QValueList of QVariant's.
             *
             * \throw TypeException If the convert failed.
             * \param object The object to convert.
             * \return The to a QValueList converted object.
             */
            static QValueList<QVariant> toList(Object::Ptr object);

    };

}}

#endif

