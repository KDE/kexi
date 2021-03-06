/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 * Copyright (C) 2014-2018 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KEXIREPORTVIEW_H
#define KEXIREPORTVIEW_H

#include <config-kreport.h>
#include <KReportRendererBase>

#include <core/KexiView.h>
#include <core/KexiRecordNavigatorHandler.h>
#include "kexireportpart.h"

class KexiDBReportDataSource;
class KReportPreRenderer;
class ORODocument;
class KReportView;
//! @todo KEXI3 class KexiScriptAdaptor;
class KRScriptFunctions;
#ifndef KEXI_MOBILE
class KexiRecordNavigator;
#endif

/**
*/
class KexiReportView : public KexiView, public KexiRecordNavigatorHandler
{
    Q_OBJECT
public:
    explicit KexiReportView(QWidget *parent);

    ~KexiReportView();

    virtual tristate afterSwitchFrom(Kexi::ViewMode mode) override;
    virtual tristate beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore) override;

    virtual void addNewRecordRequested() override;
    virtual void moveToFirstRecordRequested() override;
    virtual void moveToLastRecordRequested() override;
    virtual void moveToNextRecordRequested() override;
    virtual void moveToPreviousRecordRequested() override;
    virtual void moveToRecordRequested(int r) override;
    virtual int currentRecord() const override;
    virtual int recordCount() const override;

private:
    KReportPreRenderer *m_preRenderer;
    KReportView *m_reportView;

#ifndef KEXI_MOBILE
    KexiRecordNavigator *m_pageSelector;
#endif

    KexiReportPartTempData* tempData() const;
    KexiDBReportDataSource* createDataSource(const QDomElement &e);
    //! @todo KEXI3 KexiScriptAdaptor *m_kexi;
    KRScriptFunctions *m_functions;
    KReportRendererFactory m_factory;

    QUrl getExportUrl(const QString &mimetype, const QString &caption,
                      const QString &lastExportPathOrVariable, const QString &extension);

private Q_SLOTS:
    void slotPrintReport();
    void slotExportAsPdf();
    void slotExportAsWebPage();
#ifdef KEXI_SHOW_UNFINISHED
    void slotExportAsSpreadsheet();
    void slotExportAsTextDocument();
#endif
    void openExportedDocument(const QUrl &destination);
    void finishedAllASyncItems();
};

#endif
