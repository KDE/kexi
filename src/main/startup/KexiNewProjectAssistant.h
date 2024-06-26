/* This file is part of the KDE project
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2012 Dimitrios T. Tanis <dimitrios.tanis@kdemail.net>
   Copyright (C) 2014 Roman Shtemberko <shtemberko@gmail.com>

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

#ifndef KEXINEWPROJECTASSISTANT_H
#define KEXINEWPROJECTASSISTANT_H

#include "KexiAssistantMessageHandler.h"
#include "ui_KexiProjectStorageTypeSelectionPage.h"
#include <core/kexidbconnectionset.h>
#include <kexiutils/KexiContextMessage.h>
#include <kexiutils/KexiAssistantPage.h>
#include <kexiutils/KexiAssistantWidget.h>
#include <kexiutils/KexiCategorizedView.h>
#include <widget/KexiServerDriverNotFoundMessage.h>

#include <KDbConnectionData>
#include <KDbMessageHandler>
#include <KDbResult>

#include <QPointer>

class KexiConnectionSelectorWidget;
class KexiProjectSelectorWidget;

class KexiTemplateSelectionPage : public KexiAssistantPage
{
    Q_OBJECT
public:
    explicit KexiTemplateSelectionPage(QWidget* parent = nullptr);

    QString selectedTemplate;
    QString selectedCategory;

protected Q_SLOTS:
    void slotItemClicked(const QModelIndex& index);
private:
    KexiCategorizedView* m_templatesList;
};

class KexiProjectStorageTypeSelectionPage : public KexiAssistantPage,
                                            public Ui::KexiProjectStorageTypeSelectionPage
{
    Q_OBJECT
public:
    explicit KexiProjectStorageTypeSelectionPage(QWidget* parent = nullptr);
    virtual ~KexiProjectStorageTypeSelectionPage();

    enum class Type {
        None, //!< No type selected
        File,
        Server
    };

    /**
     * Returns selected connection type
     *
     * Selection depends on button that is focused.
     */
    Type selectedType() const;

private Q_SLOTS:
    void buttonClicked();
};

class KexiDBCaptionPage;
class KexiStartupFileHandler;

class KexiProjectCaptionSelectionPage : public KexiAssistantPage
{
    Q_OBJECT
public:
    explicit KexiProjectCaptionSelectionPage(QWidget* parent = nullptr);
    virtual ~KexiProjectCaptionSelectionPage();

    bool isAcceptable();

    KexiDBCaptionPage* contents;
    KexiStartupFileHandler *fileHandler;
    QPointer<KexiContextMessageWidget> messageWidget;
private Q_SLOTS:
    void captionTextChanged(const QString &text);
    void askForOverwriting(const KexiContextMessage& message);
private:
    void updateUrl();
};

class QProgressBar;

class KexiProjectCreationPage : public KexiAssistantPage
{
    Q_OBJECT
public:
    explicit KexiProjectCreationPage(QWidget* parent = nullptr);
    virtual ~KexiProjectCreationPage();

    QProgressBar* m_progressBar;
};

class KexiProjectConnectionSelectionPage : public KexiAssistantPage
{
    Q_OBJECT
public:
    explicit KexiProjectConnectionSelectionPage(QWidget* parent = nullptr);
    virtual ~KexiProjectConnectionSelectionPage();

    KexiConnectionSelectorWidget* connSelector;
private:
    QPointer<KexiServerDriverNotFoundMessage> m_errorMessagePopup;
};

class KexiServerDBNamePage;
class KexiGUIMessageHandler;
class KexiProjectData;
class KexiProjectSet;
class KexiProjectSelectorWidget;
class KexiNewProjectAssistant;
class KexiProjectDatabaseNameSelectionPage : public KexiAssistantPage
{
    Q_OBJECT
public:
    explicit KexiProjectDatabaseNameSelectionPage(
        KexiNewProjectAssistant* parent);
    virtual ~KexiProjectDatabaseNameSelectionPage();

    bool setConnection(KDbConnectionData* data);

    KexiServerDBNamePage* contents;
    //! @todo KEXI3 use equivalent of QPointer<KDbConnectionData>
    KDbConnectionData* conndataToShow;
    QPointer<KexiContextMessageWidget> messageWidget;
    bool isAcceptable();

private Q_SLOTS:
    void slotCaptionChanged(const QString &capt);
    void slotNameChanged(const QString &);
    void overwriteActionTriggered();

private:
    QString enteredDbName() const;
    KexiNewProjectAssistant* m_assistant;
    KexiProjectSet *m_projectSetToShow;
    KexiProjectSelectorWidget* m_projectSelector;

    bool m_dbNameAutofill;
    bool m_le_dbname_txtchanged_enabled;
    KexiProjectData* m_projectDataToOverwrite;
    QAction* m_messageWidgetActionYes;
    QAction* m_messageWidgetActionNo;
};

class KexiProjectData;

class KexiNewProjectAssistant : public KexiAssistantWidget,
                                public KexiAssistantMessageHandler,
                                public KDbResultable
{
    Q_OBJECT
public:
    explicit KexiNewProjectAssistant(QWidget* parent = nullptr);
    ~KexiNewProjectAssistant();

public Q_SLOTS:
    virtual void nextPageRequested(KexiAssistantPage* page) override;
    virtual void cancelRequested(KexiAssistantPage* page) override;
    void tryAgainActionTriggered();
    void cancelActionTriggered();

Q_SIGNALS:
    void createProject(const KexiProjectData &data);

protected:
    virtual const QWidget* calloutWidget() const override;

private:
    void createProject(
        const KDbConnectionData& cdata, const QString& databaseName,
        const QString& caption);

    class Private;
    Private* const d;
};

#endif
