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

#include "kexireportview.h"
#include <KReportView>
#include "KexiDBReportDataSource.h"
#ifndef KEXI_MOBILE
#include <widget/utils/kexirecordnavigator.h>
#endif
#include <core/KexiWindow.h>
#include <core/KexiMainWindowIface.h>
#include <KexiIcon.h>
#include <KexiStyle.h>

//! @todo KEXI3 include "../scripting/kexiscripting/kexiscriptadaptor.h"

#include <KReportPage>
#include <KReportRenderObjects>
#include <KReportPreRenderer>
#include <KReportScriptHandler>
#include "krscriptfunctions.h"

#include <KRun>
#include <KActionMenu>
#include <KMessageBox>
#include <KFileWidget>

#include <QAbstractScrollArea>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QLayout>
#include <QMimeDatabase>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>

KexiReportView::KexiReportView(QWidget *parent)
        : KexiView(parent), m_preRenderer(0), m_functions(0) //! @todo KEXI3, m_kexi(0)
{
    setObjectName("KexiReportDesigner_DataView");

    m_reportView = new KReportView(this);
    setViewWidget(m_reportView);
    KexiStyle::setupFrame(m_reportView->scrollArea());

#ifndef KEXI_MOBILE
    m_pageSelector = new KexiRecordNavigator(*m_reportView->scrollArea(), m_reportView);
    m_pageSelector->setInsertingButtonVisible(false);
    m_pageSelector->setInsertingEnabled(false);
    m_pageSelector->setLabelText(xi18nc("Page selector label", "Page:"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonFirst, xi18n("Go to first page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonFirst, xi18n("Goes to first page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonPrevious, xi18n("Go to previous page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonPrevious, xi18n("Goes to previous page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonNext, xi18n("Go to next page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonNext, xi18n("Goes to next page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonLast, xi18n("Go to last page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonLast, xi18n("Goes to last page"));
    m_pageSelector->setNumberFieldToolTips(xi18n("Current page number"), xi18n("Number of pages"));
    m_pageSelector->setRecordHandler(this);
#endif

    // -- setup local actions
    QList<QAction*> viewActions;
    QAction* a;

#ifndef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("document-print"), xi18n("Print"), this));
    a->setObjectName("print_report");
    a->setToolTip(xi18n("Print report"));
    a->setWhatsThis(xi18n("Prints the current report."));
    connect(a, SIGNAL(triggered()), this, SLOT(slotPrintReport()));

    KActionMenu *exportMenu = new KActionMenu(koIcon("document-export"), xi18nc("@title:menu","E&xport As"), this);
    exportMenu->setObjectName("report_export_as");
    exportMenu->setDelayed(false);
#endif

#ifdef KEXI_SHOW_UNFINISHED
#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(xi18n("Export:"), this));
    a->setEnabled(false); //!TODO this is a bit of a dirty way to add what looks like a label to the toolbar!
    // " ", not "", is said to be needed in maemo, the icon didn't display properly without it
    viewActions << (a = new QAction(koIcon("application-vnd.oasis.opendocument.text"), QLatin1String(" "), this));
#else

    exportMenu->addAction(a = new QAction(koIcon("application-vnd.oasis.opendocument.text"),
                                          xi18nc("open dialog to export as text document", "Text Document..."), this));
#endif
    a->setObjectName("export_as_text_document");
    a->setToolTip(xi18n("Export the report as a text document (in OpenDocument Text format)"));
    a->setWhatsThis(xi18n("Exports the report as a text document (in OpenDocument Text format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsTextDocument()));
#endif

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("application-pdf"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("application-pdf"),
                                          xi18nc("Portable Document Format...", "PDF..."), this));
#endif
    a->setObjectName("export_as_pdf");
    a->setToolTip(xi18n("Export as PDF"));
    a->setWhatsThis(xi18n("Exports the current report as PDF."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsPdf()));

#ifdef KEXI_SHOW_UNFINISHED
#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("application-vnd.oasis.opendocument.spreadsheet"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("application-vnd.oasis.opendocument.spreadsheet"),
                                          xi18nc("open dialog to export as spreadsheet", "Spreadsheet..."), this));
#endif
    a->setObjectName("export_as_spreadsheet");
    a->setToolTip(xi18n("Export the report as a spreadsheet (in OpenDocument Spreadsheet format)"));
    a->setWhatsThis(xi18n("Exports the report as a spreadsheet (in OpenDocument Spreadsheet format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsSpreadsheet()));

#endif

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("text-html"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("text-html"),
                                          xi18nc("open dialog to export as web page", "Web Page..."), this));
#endif
    a->setObjectName("export_as_web_page");
    a->setToolTip(xi18n("Export the report as a web page (in HTML format)"));
    a->setWhatsThis(xi18n("Exports the report as a web page (in HTML format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsWebPage()));

    setViewActions(viewActions);

#ifndef KEXI_MOBILE
    // setup main menu actions
    QList<QAction*> mainMenuActions;
    mainMenuActions << exportMenu;
    setMainMenuActions(mainMenuActions);
#endif
}

KexiReportView::~KexiReportView()
{
    delete m_preRenderer;
}

void KexiReportView::slotPrintReport()
{
    QScopedPointer<KReportRendererBase> renderer(m_factory.createInstance("print"));
    if (!renderer) {
        return;
    }
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        KReportRendererContext cxt;
        QPainter painter;
        cxt.setPrinter(&printer);
        cxt.setPainter(&painter);

        if (!renderer->render(cxt, m_preRenderer->document())) {
            KMessageBox::error(this,
                               xi18n("Printing the report failed."),
                               xi18n("Print Failed"));
        }
    }
}

void KexiReportView::slotExportAsPdf()
{
    QScopedPointer<KReportRendererBase> renderer(m_factory.createInstance("print"));
    if (renderer) {
        KReportRendererContext cxt;

        cxt.setUrl(getExportUrl(QLatin1String("application/pdf"),
                                          xi18n("Export Report as PDF"),
                                          "kfiledialog:///LastVisitedPDFExportPath/",
                                          "pdf"));
        if (!cxt.url().isValid()) {
            return;
        }

        QPrinter printer;
        QPainter painter;

        printer.setOutputFileName(cxt.url().path());
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);

        painter.begin(&printer);
        cxt.setPrinter(&printer);
        cxt.setPainter(&painter);
        if (!renderer->render(cxt, m_preRenderer->document())) {
            KMessageBox::error(this,
                               xi18n("Exporting the report as PDF to %1 failed.", cxt.url().toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.url());
        }
   }
}

QUrl KexiReportView::getExportUrl(const QString &mimetype, const QString &caption,
                                  const QString &lastExportPath, const QString &extension)
{
    QString defaultSavePath;
    QString recentDirClass;
    //TODO use utils
    defaultSavePath = KFileWidget::getStartUrl(QUrl(lastExportPath), recentDirClass).toLocalFile()
        + '/' + window()->partItem()->captionOrName() + '.' + extension;

    // loop until an url has been chosen or the file selection has been cancelled
    const QMimeDatabase db;
    const QString filterString = db.mimeTypeForName(mimetype).filterString();

    return QFileDialog::getSaveFileUrl(this, caption, QUrl(defaultSavePath), filterString);
}

void KexiReportView::openExportedDocument(const QUrl &destination)
{
    const int answer =
        KMessageBox::questionYesNo(
            this,
            xi18n("Do you want to open exported document?"),
            QString(),
            KStandardGuiItem::open(),
            KStandardGuiItem::close());

    if (answer == KMessageBox::Yes) {
        (void)new KRun(destination, this->topLevelWidget());
    }
}

#ifdef KEXI_SHOW_UNFINISHED

void KexiReportView::slotExportAsSpreadsheet()
{
    QScopedPointer<KReportRendererBase> renderer(m_factory.createInstance("ods"));
    if (renderer) {
        KReportRendererContext cxt;
        cxt.setUrl(getExportUrl(QLatin1String("application/vnd.oasis.opendocument.spreadsheet"),
                                          xi18n("Export Report as Spreadsheet"),
                                          "kfiledialog:///LastVisitedODSExportPath/",
                                          "ods"));
        if (!cxt.url().isValid()) {
            return;
        }

        if (!renderer->render(cxt, m_preRenderer->document())) {
            KMessageBox::error(this,
                               xi18n("Failed to export the report as spreadsheet to %1.", cxt.url().toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.url());
        }
    }
}

void KexiReportView::slotExportAsTextDocument()
{
    QScopedPointer<KReportRendererBase> renderer(m_factory.createInstance("odt"));
    //! @todo Show error or don't show the commands to the user if the plugin isn't available.
    //!       The same for other createInstance() calls.
    if (renderer) {
        KReportRendererContext cxt;
        cxt.setUrl(getExportUrl(QLatin1String("application/vnd.oasis.opendocument.text"),
                                          xi18n("Export Report as Text Document"),
                                          "kfiledialog:///LastVisitedODTExportPath/",
                                          "odt"));
        if (!cxt.url().isValid()) {
            return;
        }

        if (!renderer->render(cxt, m_preRenderer->document())) {
            KMessageBox::error(this,
                               xi18n("Exporting the report as text document to %1 failed.", cxt.url().toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.url());
        }
    }
}
#endif

void KexiReportView::slotExportAsWebPage()
{
    const QString dialogTitle = xi18n("Export Report as Web Page");
    KReportRendererContext cxt;
    cxt.setUrl(getExportUrl(QLatin1String("text/html"),
                                      dialogTitle,
                                      "kfiledialog:///LastVisitedHTMLExportPath/",
                                      "html"));
    if (!cxt.url().isValid()) {
        return;
    }

    const int answer =
        KMessageBox::questionYesNo(
            this,
            xi18nc("@info",
                   "<para>Would you like to use Cascading Style Sheets (CSS) in the exported "
                   "web page or use HTML tables?</para>"
                   "<para><note>CSS give output closer to the original.</note></para>"),
            dialogTitle,
            KGuiItem(xi18nc("@action:button", "Use CSS")),
            KGuiItem(xi18nc("@action:button", "Use Table")));

    QScopedPointer<KReportRendererBase> renderer(
         m_factory.createInstance(answer == KMessageBox::Yes ? "htmlcss" : "htmltable"));
    if (!renderer) {
        return;
    }

    if (!renderer->render(cxt, m_preRenderer->document())) {
        KMessageBox::error(this,
                           xi18n("Exporting the report as web page to %1 failed.", cxt.url().toDisplayString()),
                           xi18n("Export Failed"));
    } else {
        openExportedDocument(cxt.url());
    }
}

tristate KexiReportView::beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore)
{
    Q_UNUSED(mode);
    Q_UNUSED(dontStore);

    return true;
}

tristate KexiReportView::afterSwitchFrom(Kexi::ViewMode mode)
{
    Q_UNUSED(mode);

    if (tempData()->reportSchemaChangedInPreviousView) {
        tempData()->reportSchemaChangedInPreviousView = false;

        delete m_preRenderer;

        //qDebug() << tempData()->reportDefinition.tagName();

        m_preRenderer = new KReportPreRenderer(tempData()->reportDefinition);
        if (m_preRenderer->isValid()) {
            KexiDBReportDataSource *reportData = nullptr;
            if (!tempData()->connectionDefinition.isNull())  {
                reportData = createDataSource(tempData()->connectionDefinition);
            }
            m_preRenderer->setDataSource(reportData);
            m_preRenderer->setScriptSource(qobject_cast<KexiReportPart*>(part()));

            m_preRenderer->setName(window()->partItem()->name());

            //Add a kexi object to provide kexidb and extra functionality
//! @todo KEXI3 if we want this            if(!m_kexi) {
//                m_kexi = new KexiScriptAdaptor();
//            }
//            m_preRenderer->registerScriptObject(m_kexi, "Kexi");
            //If using a kexidb source, add a functions scripting object
            if (reportData && tempData()->connectionDefinition.attribute("type") == "internal") {
                m_functions = new KRScriptFunctions(reportData);

                m_preRenderer->registerScriptObject(m_functions, "field");
                connect(m_preRenderer, SIGNAL(groupChanged(QMap<QString, QVariant>)),
                        m_functions, SLOT(setGroupData(QMap<QString, QVariant>)));
            }
            connect(m_preRenderer, SIGNAL(finishedAllASyncItems()), this, SLOT(finishedAllASyncItems()));

            if (!m_preRenderer->generateDocument()) {
                qWarning() << "Could not generate report document";
                return false;
            }

            m_reportView->setDocument(m_preRenderer->document());
#ifndef KEXI_MOBILE
            m_pageSelector->setRecordCount(m_reportView->pageCount());
            m_pageSelector->setCurrentRecordNumber(1);
#endif
        } else {
            KMessageBox::error(this, xi18n("Report schema appears to be invalid or corrupt"), xi18n("Opening failed"));
        }
    }
    return true;
}

KexiDBReportDataSource* KexiReportView::createDataSource(const QDomElement &e)
{
    if (e.attribute("type") == "internal" && !e.attribute("source").isEmpty()) {
        return new KexiDBReportDataSource(e.attribute("source"), e.attribute("class"), tempData());
    }
    return nullptr;
}

KexiReportPartTempData* KexiReportView::tempData() const
{
    return static_cast<KexiReportPartTempData*>(window()->data());
}

void KexiReportView::addNewRecordRequested()
{

}

void KexiReportView::moveToFirstRecordRequested()
{
    m_reportView->moveToFirstPage();
#ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
#endif
}

void KexiReportView::moveToLastRecordRequested()
{
    m_reportView->moveToLastPage();
#ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
#endif
}

void KexiReportView::moveToNextRecordRequested()
{
    m_reportView->moveToNextPage();
#ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
#endif
}

void KexiReportView::moveToPreviousRecordRequested()
{
    m_reportView->moveToPreviousPage();
#ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
#endif
}

void KexiReportView::moveToRecordRequested(int r)
{
#ifdef KEXI_MOBILE
    m_reportView->moveToPage(r + 1);
#else
    // set in the navigator widget first, this will fix up the value it it's too small or large
    m_pageSelector->setCurrentRecordNumber(r + 1);
    m_reportView->moveToPage(m_pageSelector->currentRecordNumber());
#endif
}

int KexiReportView::currentRecord() const
{
    return m_reportView->currentPage();
}

int KexiReportView::recordCount() const
{
    return m_reportView->pageCount();
}

void KexiReportView::finishedAllASyncItems()
{
    m_reportView->refreshCurrentPage();
}
