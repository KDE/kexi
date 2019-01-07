/* This file is part of the KDE project
   Copyright (C) 2006-2016 Jarosław Staniek <staniek@kde.org>

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

#include "kexidatetimeformatter.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDebug>
#include <QLineEdit>
#include <QLocale>

namespace {
    const QString INPUT_MASK_BLANKS_FORMAT(QLatin1String(";_"));

    //! Like replace(const QString &before, QLatin1String after) but also returns
    //! @c true if replacement has been made.
    bool tryReplace(QString *str, const char *from, const char *to) {
        Q_ASSERT(str);
        if (str->contains(QLatin1String(from))) {
            str->replace(QLatin1String(from), QLatin1String(to));
            return true;
        }
        return false;
    }

    //! Settings-related values - cached
    class KexiDateFormatterSettings
    {
    public:
        KexiDateFormatterSettings()
        {
            KConfigGroup generalGroup(KSharedConfig::openConfig(), "General");
            allowTwoDigitYearFormats = generalGroup.readEntry("AllowTwoDigitYearFormats", false);
        }
        bool allowTwoDigitYearFormats;
        bool displayInfoOnce_allowTwoDigitYearFormats = true;
    };
    Q_GLOBAL_STATIC(KexiDateFormatterSettings, g_kexiDateFormatterSettings)
}


class Q_DECL_HIDDEN KexiDateFormatter::Private
{
public:
    Private()
        // use "short date" format system settings
        //! @note Qt has broken support for some time formats: https://bugreports.qt.io/browse/QTBUG-59382
        //!       en_DK should be yyyy-MM-dd but it's dd/MM/yyyy in Qt 5.
        //!       Use en_SE as a workaround.
        //! @todo allow to override the format using column property and/or global app settings
        : originalFormat(QLocale().dateFormat(QLocale::ShortFormat))
    {
        inputFormat = originalFormat;
        outputFormat = originalFormat;
        //qDebug() << inputFormat << QLocale().dateFormat(QLocale::LongFormat);
        emptyFormat = originalFormat;
        inputMask = originalFormat;
        computeDaysFormatAndMask();
        computeMonthsFormatAndMask();
        computeYearsFormatAndMask();
        inputMask += INPUT_MASK_BLANKS_FORMAT;
    }

    //! Original format.
    const QString originalFormat;

    //! Input mask generated using the formatter settings. Can be used in QLineEdit::setInputMask().
    QString inputMask;

    //! Date format used by fromString() and stringToVariant()
    QString inputFormat;

    //! Date format used by toString()
    QString outputFormat;

    //! Date format used by isEmpty()
    QString emptyFormat;

private:
    void computeDaysFormatAndMask() {
        // day (note, order or lookup is important):
        // - dddd - named days not supported, fall back to "d":
        if (tryReplace(&inputMask, "dddd", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("dddd"), QLatin1String("d"));
            emptyFormat.remove(QLatin1String("dddd"));
            return;
        }
        // - ddd - named days not supported, fall back to "d":
        if (tryReplace(&inputMask, "ddd", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("ddd"), QLatin1String("d"));
            emptyFormat.remove(QLatin1String("ddd"));
            return;
        }
        // - dd - The day as a number with a leading zero (01 to 31)
        //        second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "dd", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("dd"), QLatin1String("d"));
            emptyFormat.remove(QLatin1String("dd"));
            return;
        }
        // - d - The day as a number without a leading zero (1 to 31);
        //       second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "d", "90")) {
            emptyFormat.remove(QLatin1String("d"));
            return;
        }
        qWarning() << "Not found 'days' part in format" << inputFormat;
    }
    void computeMonthsFormatAndMask() {
        // month (note, order or lookup is important):
        // - MMMM - named months not supported, fall back to "M"
        if (tryReplace(&inputMask, "MMMM", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("MMMM"), QLatin1String("M"));
            emptyFormat.remove(QLatin1String("MMMM"));
            return;
        }
        // - MMM - named months not supported, fall back to "M"
        if (tryReplace(&inputMask, "MMM", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("MMM"), QLatin1String("M"));
            emptyFormat.remove(QLatin1String("MMM"));
            return;
        }
        // - MM - The month as a number with a leading zero (01 to 12)
        //        second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "MM", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("MM"), QLatin1String("M"));
            emptyFormat.remove(QLatin1String("MM"));
            return;
        }
        // - M - The month as a number without a leading zero (1 to 12);
        //       second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "M", "90")) {
            emptyFormat.remove(QLatin1String("M"));
            return;
        }
        qWarning() << "Not found 'months' part in format" << inputFormat;
    }
    void computeYearsFormatAndMask() {
        // - yyyy - The year as four digit number.
        const char *longDigits = "9999";
        if (tryReplace(&inputMask, "yyyy", longDigits)) {
            emptyFormat.remove(QLatin1String("yyyy"));
            return;
        }
        const char *shortYearDigits
            = g_kexiDateFormatterSettings->allowTwoDigitYearFormats ? "99" : longDigits;
        // - yy - The year as two digit number.
        if (tryReplace(&inputMask, "yy", shortYearDigits)) {
            emptyFormat.remove(QLatin1String("yy"));
            if (!g_kexiDateFormatterSettings->allowTwoDigitYearFormats) {
                // change input format too
                inputFormat.replace(QLatin1String("yy"), QLatin1String("yyyy"));
                // change output format too
                outputFormat.replace(QLatin1String("yy"), QLatin1String("yyyy"));
                if (g_kexiDateFormatterSettings->displayInfoOnce_allowTwoDigitYearFormats) {
                    qInfo() << qPrintable(
                                   QStringLiteral(
                                       "Two-digit year formats for dates are not allowed so KEXI "
                                       "will alter "
                                       "date format \"%1\" by replacing two-digits years with "
                                       "four-digits for accuracy. New input format is \"%2\", new "
                                       "input mask is \"%3\" and new output format is \"%4\".")
                                       .arg(originalFormat, inputFormat, inputMask, outputFormat))
                            << "This change will affect input and display. Set the "
                               "General/AllowTwoDigitYearFormats option to true to enable use of "
                               "two-digit year formats.";
                    g_kexiDateFormatterSettings->displayInfoOnce_allowTwoDigitYearFormats = false;
                }
            }
            return;
        }
        qWarning() << "Not found 'years' part in format" << inputFormat;
    }
};

class Q_DECL_HIDDEN KexiTimeFormatter::Private
{
public:
    Private()
        // use "short date" format system settings
        //! @todo allow to override the format using column property and/or global app settings
        : inputFormat(QLocale().timeFormat(QLocale::ShortFormat))
    {
        outputFormat = inputFormat;
        emptyFormat = inputFormat;
        inputMask = inputFormat;
        computeHoursFormatAndMask();
        computeMinutesFormatAndMask();
        computeSecondsFormatAndMask();
        computeMillisecondsFormatAndMask();
        computeAmPmFormatAndMask();
        inputMask += INPUT_MASK_BLANKS_FORMAT;
    }

    ~Private() {
    }

    //! Input mask generated using the formatter settings. Can be used in QLineEdit::setInputMask().
    QString inputMask;

    //! Time format used by fromString() and stringToVariant()
    QString inputFormat;

    //! Time format used by toString()
    QString outputFormat;

    //! Date format used by isEmpty()
    QString emptyFormat;

private:
    void computeHoursFormatAndMask() {
        // - hh - the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display).
        //        second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "hh", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("hh"), QLatin1String("h"));
            emptyFormat.remove(QLatin1String("hh"));
            return;
        }
        // the same for HH
        if (tryReplace(&inputMask, "HH", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("HH"), QLatin1String("h"));
            emptyFormat.remove(QLatin1String("HH"));
            return;
        }
        // - h - the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display).
        //       second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "h", "90")) {
            emptyFormat.remove(QLatin1String("h"));
            return;
        }
        // the same for H
        if (tryReplace(&inputMask, "H", "90")) {
            emptyFormat.remove(QLatin1String("H"));
            return;
        }
        qWarning() << "Not found 'hours' part in format" << inputFormat;
    }
    void computeMinutesFormatAndMask() {
        // - mm - the minute with a leading zero (00 to 59).
        if (tryReplace(&inputMask, "mm", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("mm"), QLatin1String("m"));
            emptyFormat.remove(QLatin1String("mm"));
            return;
        }
        // - m - the minute without a leading zero (0 to 59).
        //       second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "m", "90")) {
            emptyFormat.remove(QLatin1String("m"));
            return;
        }
        qWarning() << "Not found 'minutes' part in format" << inputFormat;
    }
    void computeSecondsFormatAndMask() {
        // - ss - the second with a leading zero (00 to 59).
        //        second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "ss", "90")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("ss"), QLatin1String("s"));
            emptyFormat.remove(QLatin1String("ss"));
            return;
        }
        // - s - the second without a leading zero (0 to 59).
        //       second character is optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "s", "90")) {
            emptyFormat.remove(QLatin1String("s"));
            return;
        }
        //qDebug() << "Not found 'seconds' part in format" << inputFormat;
    }
    void computeMillisecondsFormatAndMask() {
        // - zzz - the milliseconds with leading zeroes (000 to 999).
        //       last two characters are optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "zzz", "900")) {
            // also replace the input format
            inputFormat.replace(QLatin1String("zzz"), QLatin1String("z"));
            emptyFormat.remove(QLatin1String("zzz"));
            return;
        }
        // - m - the milliseconds without leading zeroes (0 to 999).
        //       last two characters are optional, e.g. 1_ is OK
        if (tryReplace(&inputMask, "z", "900")) {
            emptyFormat.remove(QLatin1String("z"));
            return;
        }
        //qDebug() << "Not found 'milliseconds' part in format" << inputFormat;
    }
    void computeAmPmFormatAndMask() {
        // - AP - interpret as an AM/PM time. AP must be either "AM" or "PM".
        //! @note not a 100% accurate approach, we're assuming that "AP" substring is only
        //!       used to indicate AM/PM
        if (tryReplace(&inputMask, "AP", ">AA!")) { // we're also converting to upper case
            emptyFormat.remove(QLatin1String("AP"));
            return;
        }
        // - ap - interpret as an AM/PM time. ap must be either "am" or "pm".
        //! @note see above
        if (tryReplace(&inputMask, "ap", "<AA!")) { // we're also converting to upper case
            emptyFormat.remove(QLatin1String("ap"));
            return;
        }
        //qDebug() << "Not found 'AM/PM' part in format" << inputFormat;
    }
};

KexiDateFormatter::KexiDateFormatter()
  : d(new Private)
{
}

KexiDateFormatter::~KexiDateFormatter()
{
    delete d;
}

QDate KexiDateFormatter::fromString(const QString& str) const
{
    return QDate::fromString(str, d->inputFormat);
}

QVariant KexiDateFormatter::stringToVariant(const QString& str) const
{
    const QDate date(fromString(str));
    return date.isValid() ? date : QVariant();
}

bool KexiDateFormatter::isEmpty(const QString& str) const
{
    const QString t(str.trimmed());
    return t.isEmpty() || t == d->emptyFormat;
}

QString KexiDateFormatter::inputMask() const
{
    return d->inputMask;
}

QString KexiDateFormatter::toString(const QDate& date) const
{
    return date.toString(d->outputFormat);
}

//------------------------------------------------

KexiTimeFormatter::KexiTimeFormatter()
        : d(new Private)
{
}

KexiTimeFormatter::~KexiTimeFormatter()
{
    delete d;
}

QTime KexiTimeFormatter::fromString(const QString& str) const
{
    return QTime::fromString(str, d->inputFormat);
}

QVariant KexiTimeFormatter::stringToVariant(const QString& str)
{
    const QTime result(fromString(str));
    return result.isValid() ? result : QVariant();
}

bool KexiTimeFormatter::isEmpty(const QString& str) const
{
    const QString t(str.trimmed());
    return t.isEmpty() || t == d->emptyFormat;
}

QString KexiTimeFormatter::toString(const QTime& time) const
{
    return time.toString(d->outputFormat);
}

QString KexiTimeFormatter::inputMask() const
{
    return d->inputMask;
}

//------------------------------------------------

QString KexiDateTimeFormatter::inputMask(const KexiDateFormatter& dateFormatter,
                                       const KexiTimeFormatter& timeFormatter)
{
    QString mask(dateFormatter.inputMask());
    mask.chop(INPUT_MASK_BLANKS_FORMAT.length());
    return mask + " " + timeFormatter.inputMask();
}

QDateTime KexiDateTimeFormatter::fromString(
    const KexiDateFormatter& dateFormatter,
    const KexiTimeFormatter& timeFormatter, const QString& str)
{
    QString s(str.trimmed());
    const int timepos = s.indexOf(' ');
    const bool emptyTime = timepos >= 0 && timeFormatter.isEmpty(s.mid(timepos + 1));
    if (emptyTime)
        s = s.left(timepos);
    if (timepos > 0 && !emptyTime) {
        return QDateTime(
                   dateFormatter.fromString(s.left(timepos)),
                   timeFormatter.fromString(s.mid(timepos + 1))
               );
    } else {
        return QDateTime(
                   dateFormatter.fromString(s),
                   QTime(0, 0, 0)
               );
    }
}

QString KexiDateTimeFormatter::toString(const KexiDateFormatter &dateFormatter,
                                        const KexiTimeFormatter &timeFormatter,
                                        const QDateTime &value)
{
    if (value.isValid())
        return dateFormatter.toString(value.date()) + ' '
               + timeFormatter.toString(value.time());
    return QString();
}

bool KexiDateTimeFormatter::isEmpty(const KexiDateFormatter& dateFormatter,
                                    const KexiTimeFormatter& timeFormatter,
                                    const QString& str)
{
    int timepos = str.indexOf(' ');
    const bool emptyTime = timepos >= 0 && timeFormatter.isEmpty(str.mid(timepos + 1));
    return timepos >= 0 && dateFormatter.isEmpty(str.left(timepos)) && emptyTime;
}

bool KexiDateTimeFormatter::isValid(const KexiDateFormatter& dateFormatter,
                                    const KexiTimeFormatter& timeFormatter, const QString& str)
{
    int timepos = str.indexOf(' ');
    const bool emptyTime = timepos >= 0 && timeFormatter.isEmpty(str.mid(timepos + 1));
    if (timepos >= 0 && dateFormatter.isEmpty(str.left(timepos))
            && emptyTime)
    {
        //empty date/time is valid
        return true;
    }
    return timepos >= 0 && dateFormatter.fromString(str.left(timepos)).isValid()
           && (emptyTime /*date without time is also valid*/ || timeFormatter.fromString(str.mid(timepos + 1)).isValid());
}
