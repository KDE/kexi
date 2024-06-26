/* This file is part of the KDE project
   Copyright (C) 2005-2012 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXICSVIMPORTOPTIONSDLG_H
#define KEXICSVIMPORTOPTIONSDLG_H

#include <QDialog>
#include <QCheckBox>

class KexiCharacterEncodingComboBox;
class QComboBox;

//! @short CSV Options
class KexiCSVImportOptions
{
public:
    KexiCSVImportOptions();
    ~KexiCSVImportOptions();

    //! Date format values
    enum DateFormat {
        AutoDateFormat = 0, //!< auto
        DMY = 1, //!< day-month-year
        YMD = 2, //!< year-month-day
        MDY = 3  //!< month-day-year
    };

    bool operator== (const KexiCSVImportOptions & opt) const;
    bool operator!= (const KexiCSVImportOptions & opt) const;

    QString encoding;
    DateFormat dateFormat;
    bool defaultEncodingExplicitySet;
    bool trimmedInTextValuesChecked;
    bool nullsImportedAsEmptyTextChecked;
};

//! @short CSV Options dialog
class KexiCSVImportOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KexiCSVImportOptionsDialog(const KexiCSVImportOptions& options, QWidget* parent = nullptr);
    virtual ~KexiCSVImportOptionsDialog();

    KexiCSVImportOptions options() const;

protected Q_SLOTS:
    virtual void accept() override;

protected:
    KexiCharacterEncodingComboBox *m_encodingComboBox;
    QCheckBox *m_chkAlwaysUseThisEncoding;
    QCheckBox *m_chkStripWhiteSpaceInTextValues;
    QCheckBox *m_chkImportNULLsAsEmptyText;
    QComboBox *m_comboDateFormat;
};

#endif
