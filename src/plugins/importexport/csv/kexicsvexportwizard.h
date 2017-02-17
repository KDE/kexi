/* This file is part of the KDE project
   Copyright (C) 2012 Oleg Kukharchuk <oleg.kuh@gmail.org>
   Copyright (C) 2005-2013 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXI_CSVEXPORTWIZARD_H
#define KEXI_CSVEXPORTWIZARD_H

#include <KConfigGroup>
#include <KSharedConfig>
#include <KAssistantDialog>

#include "kexicsvexport.h"

class QCheckBox;
class QGroupBox;
class QPushButton;
class KexiFileWidget;
class KexiCSVDelimiterWidget;
class KexiCSVTextQuoteComboBox;
class KexiCSVInfoLabel;
class KexiCharacterEncodingComboBox;
class KPageWidgetItem;
class KDbTableOrQuerySchema;

/*! @short Kexi CSV export wizard
 Supports exporting to a file and to a clipboard. */
class KexiCSVExportWizard : public KAssistantDialog
{
    Q_OBJECT

public:
    explicit KexiCSVExportWizard(const KexiCSVExport::Options& options,
                                 QWidget * parent = 0);

    virtual ~KexiCSVExportWizard();

    bool canceled() const;

protected Q_SLOTS:
    virtual void next();
    virtual void done(int result);
    void slotShowOptionsButtonClicked();
    void slotDefaultsButtonClicked();
    void slotCurrentPageChanged(KPageWidgetItem*, KPageWidgetItem*);

protected:

    //! \return default delimiter depending on mode.
    QString defaultDelimiter() const;

    //! \return default text quote depending on mode.
    QString defaultTextQuote() const;

    //! Helper, works like KSharedConfig::openConfig()->readBoolEntry(const char*, bool) but if mode is Clipboard,
    //! "Exporting" is replaced with "Copying" and "Export" is replaced with "Copy"
    //! and "CSVFiles" is replaced with "CSVToClipboard"
    //! in \a key, to keep the setting separate.
    bool readBoolEntry(const char *key, bool defaultValue);

    //! Helper like \ref readBoolEntry(const char *, bool), but for QString values.
    QString readEntry(const char *key, const QString& defaultValue = QString());

    //! Helper, works like KSharedConfig::openConfig()->writeEntry(const char*,bool) but if mode is Clipboard,
    //! "Exporting" is replaced with "Copying" and "Export" is replaced with "Copy"
    //! and "CSVFiles" is replaced with "CSVToClipboard"
    //! in \a key, to keep the setting separate.
    void writeEntry(const char *key, bool value);

    //! Helper like \ref writeEntry(const char *, bool), but for QString values.
    void writeEntry(const char *key, const QString& value);

    //! Helper like \ref writeEntry(const char *, bool), but for deleting config entry.
    void deleteEntry(const char *key);

    KexiCSVExport::Options m_options;
    KexiFileWidget* m_fileSaveWidget = nullptr;
    QWidget* m_exportOptionsWidget = nullptr;
    KPageWidgetItem *m_fileSavePage = nullptr;
    KPageWidgetItem *m_exportOptionsPage = nullptr;
    QPushButton *m_showOptionsButton = nullptr;
    QPushButton *m_defaultsBtn = nullptr;
    QGroupBox* m_exportOptionsSection = nullptr;
    KexiCSVInfoLabel *m_infoLblFrom = nullptr;
    KexiCSVInfoLabel *m_infoLblTo = nullptr;
    KexiCSVDelimiterWidget* m_delimiterWidget = nullptr;
    KexiCSVTextQuoteComboBox* m_textQuote = nullptr;
    KexiCharacterEncodingComboBox *m_characterEncodingCombo = nullptr;
    QCheckBox *m_addColumnNamesCheckBox = nullptr;
    QCheckBox *m_alwaysUseCheckBox = nullptr;
    KDbTableOrQuerySchema* m_tableOrQuery;
    KConfigGroup m_importExportGroup;
    bool m_canceled = false;
};

#endif
