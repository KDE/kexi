/* This file is part of the KDE project
   Copyright (C) 2007-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXITEXTFORMATTER_H
#define KEXITEXTFORMATTER_H

#include "kexidatatable_export.h"

#include <KDbField>

//! @short Text formatter used to format QVariant values to text for displaying and back to QVariant
/*! Used by KexiInputTableEdit, KexiDateTableEdit, KexiTimeTableEdit, KexiDateTimeTableEdit,
 KexiDBLineEdit (forms), etc. */
class KEXIDATATABLE_EXPORT KexiTextFormatter
{
public:
    KexiTextFormatter();
    ~KexiTextFormatter();

    //! Assigns \a field to the formatter. This affects its behaviour.
    void setField(const KDbField* field);

    //! @see KexiTextFormatter::setOverrideDecimalPlaces()
    class OverrideDecimalPlaces
    {
    public:
        OverrideDecimalPlaces() : enabled(false), value(-1) {}
        bool enabled;
        int value;
    };

    //! Overrides number of decimal places returned KDbField::visibleDecimalPlaces().
    //! Affects toString() when field type is @c Float or @c Double.
    //! See documentation of KDb::numberToString() for information on possible values.
    void setOverrideDecimalPlaces(const OverrideDecimalPlaces& overrideDecimalPlaces);

    //! @return override for number of decimal places.
    //! Affects toString() when field type is @c Float or @c Double.
    OverrideDecimalPlaces overridesDecimalPlaces() const;

    //! Enables or disables thousands separators for converting floating point numbers
    //! to string using toString().
    void setGroupSeparatorsEnabled(bool set);

    //! @return true if thousands separators is used for converting floating point numbers
    //! to string using toString().
    bool isGroupSeparatorsEnabled() const;

    /*! \return string converted from \a value.
     A field schema set using setField() is used to perform the formatting.
     \a add is a text that should be added to the value if possible.
     Used in KexiInputTableEdit::setValueInternal(), by form widgets and for reporting/printing. */
    QString toString(const QVariant& value, const QString& add, bool *lengthExceeded) const;

    /*! \return variant value converted from \a text
     A field schema set using setField() is used to perform the formatting.
     @a *ok is set to @c true on success and to @c false on failure.
     On failure null QVariant is always returned.
     This method is used in KexiInputTableEdit::value(), by form widgets and for reporting/printing. */
    QVariant fromString(const QString& text, bool *ok = nullptr) const;

    /*! \return true if value formatted as \a text is empty.
     A field schema set using setField() is used to perform the calculation. */
    bool valueIsEmpty(const QString& text) const;

    /*! \return true if value formatted as \a text is valid.
     A field schema set using setField() is used to perform the calculation. */
    bool valueIsValid(const QString& text) const;

    /*! \return input mask for intering values related to a field schema
     which has been set using setField(). */
    QString inputMask() const;

    /*! \return true if @a text length is exceeded. */
    bool lengthExceeded(const QString& text) const;

    class Private;
    Private * const d;
};

#endif
