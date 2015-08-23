/* This file is part of the KDE project
   Copyright (C) 2006-2011 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDATETIMEFORMATTER_H
#define KEXIDATETIMEFORMATTER_H

#include "kexiguiutils_export.h"


//! @short Date formatter used by KexiDateTableEdit and KexiDateTimeTableEdit
class KEXIGUIUTILS_EXPORT KexiDateFormatter
{
public:
    //! Creates new formatter with KDE setting for "short date"
    KexiDateFormatter();

    //! Creates new formatter with given settings
//! @todo  KexiDateFormatter(... settings ...);

    ~KexiDateFormatter();

    //! Converts string \a str to date using predefined settings.
    //! \return invalid date if the conversion is impossible
    QDate fromString(const QString& str) const;

    /*! Converts string \a str to date using predefined settings
     and returns QVariant containing the date value.
     This method does the same as fromString(const QString&) but if \a string
     contains invalid date representation, e.g. contains only spaces
     and separators, null QVariant() is returned. */
    QVariant stringToVariant(const QString& str) const;

    //! Converts \a date to string using predefined settings.
    //! \return null string if \a date is invalid
    QString toString(const QDate& date) const;

    //! \return Input mask generated using the formatter settings.
    //! Can be used in QLineEdit::setInputMask().
    QString inputMask() const;

    //! \return separator for this date format, a single character like "-" or "/"
    QString separator() const;

    //! \return true if \a str contains only spaces
    //! and separators according to the date format.
    bool isEmpty(const QString& str) const;

protected:
    //! Order of date's components (day, month, year)
    enum Order { DMY, MDY, YMD, YDM };

private:
    class Private;
    Private * const d;
};


/*! @short Time formatter used by KexiTimeTableEdit and KexiDateTimeTableEdit
 Following time formats are allowed: HH:MM:SS (24h), HH:MM (24h), HH:MM AM/PM (12h)
 Separator MUST be ":" */
class KEXIGUIUTILS_EXPORT KexiTimeFormatter
{
public:
    //! Creates new formatter with KDE setting for time
    KexiTimeFormatter();

    //! Creates new formatter with given settings
//! @todo  KexiDateFormatter(... settings ...);

    ~KexiTimeFormatter();

    //! converts string \a str to time using predefined settings
    //! \return invalid time if the conversion is impossible
    QTime fromString(const QString& str) const;

    /*! Converts string \a str to time using predefined settings
     and returns QVariant containing the time value.
     This method does the same as fromString(const QString&) but if \a string
     contains invalid time representation, e.g. contains only spaces
     and separators, null QVariant() is returned. */
    QVariant stringToVariant(const QString& str);

    //! converts \a time to string using predefined settings
    //! \return null string if \a time is invalid
    QString toString(const QTime& time) const;

    //! \return Input mask generated using the formatter settings.
    //! Can be used in QLineEdit::setInputMask().
    QString inputMask() const;

    //! \return true if \a str contains only spaces
    //! and separators according to the time format.
    bool isEmpty(const QString& str) const;

private:
    class Private;
    Private * const d;
};

//! Date/time formatting routines
class KEXIGUIUTILS_EXPORT KexiDateTimeFormatter
{
public:
    //! \return a date/time input mask using date and time formatter.
    //! Date is separated from time by one space character.
    static QString inputMask(
        const KexiDateFormatter& dateFormatter, const KexiTimeFormatter& timeFormatter);

    /*! \return a QDateTime value converted from string using \a dateFormatter and \a timeFormatter.
    A single space between date and time is assumed.
    Invalid value is returned when \a str contains no valid date or \a str contains invalid time.
    Value with time equal 00:00:00 is returned if \a str contains empty time part. */
    static QDateTime fromString(
        const KexiDateFormatter& dateFormatter, const KexiTimeFormatter& timeFormatter,
        const QString& str);

    /*! \return a string value converted from \a value using \a dateFormatter and \a timeFormatter.
    A single space between date and time is inserted.
    Empty string is returned when \a value is invalid.
    Value with time equal 00:00:00 is returned if \a str contains empty time part. */
    static QString toString(const KexiDateFormatter &dateFormatter,
                            const KexiTimeFormatter &timeFormatter,
                            const QDateTime &value);

    /*! \return true if \a str contains only spaces and separators according to formats provided by
    \a dateFormatter and \a timeFormatter. */
    static bool isEmpty(const KexiDateFormatter& dateFormatter,
        const KexiTimeFormatter& timeFormatter, const QString& str);

    /*! \return true if \a str gives valid date/time value according to formats provided by
    \a dateFormatter and \a timeFormatter. */
    static bool isValid(const KexiDateFormatter& dateFormatter,
        const KexiTimeFormatter& timeFormatter, const QString& str);
};

#endif
