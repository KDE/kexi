/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEXIREPORTVIEW_H
#define KEXIREPORTVIEW_H

#include <QGraphicsView>

#include <KDbConnection>

#include <KoReportRendererBase>

#include <core/KexiView.h>
#include <core/KexiRecordNavigatorHandler.h>
#include "kexireportpart.h"

class KoReportPreRenderer;
class ORODocument;
class KoReportPage;
class KexiScriptAdaptor;
class KRScriptFunctions;
#ifndef KEXI_MOBILE
class KexiRecordNavigator;
#endif

/**
 @author Adam Pigg <adam@piggz.co.uk>
*/
class KexiReportView : public KexiView, public KexiRecordNavigatorHandler
{
    Q_OBJECT
public:
    explicit KexiReportView(QWidget *parent);

    ~KexiReportView();

    virtual tristate afterSwitchFrom(Kexi::ViewMode mode);
    virtual tristate beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore);

    virtual void addNewRecordRequested();
    virtual void moveToFirstRecordRequested();
    virtual void moveToLastRecordRequested();
    virtual void moveToNextRecordRequested();
    virtual void moveToPreviousRecordRequested();
    virtual void moveToRecordRequested(uint r);
    virtual int currentRecord() const;
    virtual int recordCount() const;

private:
    KoReportPreRenderer *m_preRenderer;
    ORODocument *m_reportDocument;
    QGraphicsView *m_reportView;
    QGraphicsScene *m_reportScene;
    KoReportPage *m_reportPage;

#ifndef KEXI_MOBILE
    KexiRecordNavigator *m_pageSelector;
#endif
    int m_currentPage;
    int m_pageCount;
    KexiReportPart::TempData* tempData() const;
    KoReportData* sourceData(QDomElement e);

    KexiScriptAdaptor *m_kexi;
    KRScriptFunctions *m_functions;
    KoReportRendererFactory m_factory;

    //! @todo KEXI3 add equivalent of kfiledialog:/// for lastExportPathOrVariable
    QUrl getExportUrl(const QString &mimetype, const QString &caption,
                      const QString &lastExportPathOrVariable, const QString &extension);

private Q_SLOTS:
    void slotPrintReport();
    void slotExportAsPdf();
    void slotExportAsSpreadsheet();
    void slotExportAsWebPage();
    void slotExportAsTextDocument();
    void openExportedDocument(const QUrl &destination);
};

#endif
