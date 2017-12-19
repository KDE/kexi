/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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

#include "kexiquerydesignersql.h"
#include "kexiquerydesignersqleditor.h"
#include "kexiquerypart.h"
#include "kexisectionheader.h"
#include <KexiIcon.h>
#include <kexiutils/utils.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include <KexiWindow.h>

#include <KDbConnection>
#include <KDbNativeStatementBuilder>
#include <KDbParser>
#include <KDbQuerySchema>

#include <KMessageBox>

#include <QSplitter>
#include <QTimer>
#include <QLabel>
#include <QPalette>
#include <QToolTip>
#include <QAction>
#include <QDebug>
#include <QHBoxLayout>

static bool compareSql(const QString& sql1, const QString& sql2)
{
    //! @todo use reformatting functions here
    return sql1.trimmed() == sql2.trimmed();
}

//===================

//! @internal
class Q_DECL_HIDDEN KexiQueryDesignerSqlView::Private
{
public:
    Private() :
            statusPixmapOk(koDesktopIcon("dialog-ok"))
            , statusPixmapErr(koDesktopIcon("dialog-error"))
            , statusPixmapInfo(koDesktopIcon("dialog-information"))
            , parsedQuery(0)
            , heightForStatusMode(-1)
            , justSwitchedFromNoViewMode(false)
            , slotTextChangedEnabled(true) {
    }
    ~Private() {
        delete parsedQuery;
    }
    KexiQueryDesignerSqlEditor *editor;
    QLabel *pixmapStatus, *lblStatus;
    QHBoxLayout *statusHLyr;
    QFrame *statusMainWidget;
    KexiSectionHeader *head;
    QWidget *bottomPane;
    QPixmap statusPixmapOk, statusPixmapErr, statusPixmapInfo;
    QSplitter *splitter;
    //! For internal use, this pointer is usually copied to TempData structure,
    //! when switching out of this view (then it's cleared).
    //! If it's still present at destruction of Private then it's deleted.
    KDbQuerySchema *parsedQuery;
    //! For internal use, statement passed in switching to this view
    KDbEscapedString origStatement;
    //! needed to remember height for both modes, between switching
    int heightForStatusMode;
    //! helper for beforeSwitchTo()
    bool justSwitchedFromNoViewMode;
    //! helper for slotTextChanged()
    bool slotTextChangedEnabled;
};

//===================

KexiQueryDesignerSqlView::KexiQueryDesignerSqlView(QWidget *parent)
        : KexiView(parent)
        , d(new Private())
{
    d->splitter = new QSplitter(Qt::Vertical, this);
    d->splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    d->splitter->setChildrenCollapsible(false);
    d->head = new KexiSectionHeader(xi18n("SQL Query Text"), Qt::Vertical);
    d->splitter->addWidget(d->head);
    d->splitter->setStretchFactor(
        d->splitter->indexOf(d->head), 3/*stretch*/);
    d->editor = new KexiQueryDesignerSqlEditor(d->head);
    d->editor->setObjectName("sqleditor");
    d->editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    d->head->setWidget(d->editor);
    connect(d->editor, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));

    // -- bottom pane (status)
    d->bottomPane = new QWidget;
    QVBoxLayout *bottomPaneLyr = new QVBoxLayout(d->bottomPane);
    d->splitter->addWidget(d->bottomPane);
    d->splitter->setStretchFactor(
        d->splitter->indexOf(d->bottomPane), 1/*KeepSize*/);

    // -- status pane
    d->statusMainWidget = new QFrame(d->bottomPane);
    bottomPaneLyr->addWidget(d->statusMainWidget);
    d->statusMainWidget->setAutoFillBackground(true);
    d->statusMainWidget->setFrameShape(QFrame::StyledPanel);
    d->statusMainWidget->setFrameShadow(QFrame::Plain);
    d->statusMainWidget->setBackgroundRole(QPalette::Base);
    QPalette pal(QToolTip::palette());
    pal.setBrush(QPalette::Base, QToolTip::palette().brush(QPalette::Button));
    d->statusMainWidget->setPalette(pal);

    d->statusHLyr = new QHBoxLayout(d->statusMainWidget);
    d->statusHLyr->setContentsMargins(0, KexiUtils::marginHint() / 2, 0, KexiUtils::marginHint() / 2);
    d->statusHLyr->setSpacing(0);

    d->pixmapStatus = new QLabel(d->statusMainWidget);
    d->statusHLyr->addWidget(d->pixmapStatus);
    d->pixmapStatus->setFixedWidth(d->statusPixmapOk.width()*3 / 2);
    d->pixmapStatus->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    d->pixmapStatus->setAutoFillBackground(true);

    d->lblStatus = new QLabel(d->statusMainWidget);
    d->statusHLyr->addWidget(d->lblStatus);
    d->lblStatus->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    d->lblStatus->setWordWrap(true);
    d->lblStatus->setTextInteractionFlags(Qt::TextBrowserInteraction);
    d->lblStatus->setMinimumHeight(d->statusPixmapOk.width());

    addChildView(d->editor);
    setViewWidget(d->splitter);
    d->splitter->setFocusProxy(d->editor);
    setFocusProxy(d->editor);

    // -- setup local actions
    QList<QAction*> viewActions;
    QAction* a;
    viewActions << (a = new QAction(KexiIcon("validate"), xi18n("Check Query"), this));
    a->setObjectName("querypart_check_query");
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5));
    a->setToolTip(xi18n("Check Query"));
    a->setWhatsThis(xi18n("Checks query for validity."));
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(slotCheckQuery()));

    setViewActions(viewActions);

    slotCheckQuery();
    updateGeometry();
}

KexiQueryDesignerSqlView::~KexiQueryDesignerSqlView()
{
    delete d;
}

KexiQueryDesignerSqlEditor *KexiQueryDesignerSqlView::editor() const
{
    return d->editor;
}

void KexiQueryDesignerSqlView::setStatusOk()
{
    d->pixmapStatus->setPixmap(d->statusPixmapOk);
    setStatusText("<h3>" + xi18n("The query is correct") + "</h3>");
}

void KexiQueryDesignerSqlView::setStatusError(const QString& msg)
{
    d->pixmapStatus->setPixmap(d->statusPixmapErr);
    setStatusText("<h3>" + xi18n("The query is incorrect") + "</h3><p>" + msg + "</p>");
}

void KexiQueryDesignerSqlView::setStatusEmpty()
{
    d->pixmapStatus->setPixmap(d->statusPixmapInfo);
    setStatusText(
        xi18n("Please enter your query and execute \"Check query\" function to verify it."));
}

void KexiQueryDesignerSqlView::setStatusText(const QString& text)
{
    d->lblStatus->setText(text);
}

tristate KexiQueryDesignerSqlView::beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore)
{
    Q_ASSERT(dontStore);
//! @todo
    *dontStore = true;
    if (mode == Kexi::DesignViewMode || mode == Kexi::DataViewMode) {
        QString sqlText = d->editor->text().trimmed();
        KexiQueryPartTempData * temp = tempData();
        const bool sqlTextIsEmpty = sqlText.isEmpty();
        if (sqlTextIsEmpty && mode == Kexi::DesignViewMode) {
            //special case: empty SQL text, allow to switch to the design view
            if (temp->query()) {
                temp->setQueryChangedInView(true); //query changed
                temp->setQuery(0);
            }
        }
        else {
            const bool designViewWasVisible = window()->viewForMode(mode) != 0;
            //should we check SQL text?
            if (designViewWasVisible
                    && !sqlTextIsEmpty //for empty text always show error
                    && !d->justSwitchedFromNoViewMode //unchanged, but we should check SQL text
                    && compareSql(d->origStatement.toString(), d->editor->text()))
            {
                //statement unchanged! - nothing to do
                temp->setQueryChangedInView(false);
            } else {
                //yes: parse SQL text
                if (sqlTextIsEmpty || !slotCheckQuery()) {
                    if (KMessageBox::Cancel == KMessageBox::warningContinueCancel(
                               this, xi18n("<para>The query you entered is incorrect.</para>"
                                           "<para>Do you want discard changes made to this SQL "
                                           "text and switch to the other view?</para>"),
                               QString(), KGuiItem(xi18n("Discard Changes and Switch"),
                                                   KStandardGuiItem::yes().iconName()),
                               KGuiItem(xi18n("Don't Switch"),
                                        KStandardGuiItem::cancel().iconName())))
                    {
                        return cancelled;
                    }
                    //do not change original query - it's invalid
                    temp->setQueryChangedInView(false);
                    //this view is no longer _just_ switched from "NoViewMode"
                    d->justSwitchedFromNoViewMode = false;
                    d->slotTextChangedEnabled = false;
                    d->editor->setText(d->origStatement.toString());
                    d->slotTextChangedEnabled = true;
                    slotCheckQuery();
                    return true;
                }
                //this view is no longer _just_ switched from "NoViewMode"
                d->justSwitchedFromNoViewMode = false;
                //replace old query schema with new one
                temp->setQuery(d->parsedQuery);   //this will also delete temp->query()
                d->parsedQuery = 0;
                temp->setQueryChangedInView(true);
            }
        }
        d->origStatement = KDbEscapedString(d->editor->text());
    }

    d->editor->setFocus();
    return true;
}

tristate
KexiQueryDesignerSqlView::afterSwitchFrom(Kexi::ViewMode mode)
{
    if (mode == Kexi::NoViewMode) {
        //User opened text view _directly_.
        //This flag is set to indicate for beforeSwitchTo() that even if text has not been changed,
        //SQL text should be invalidated.
        d->justSwitchedFromNoViewMode = true;
    }
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiQueryPartTempData * temp = tempData();
    KDbQuerySchema *query = temp->query();
    if (!query) {//try to just get saved schema, instead of temporary one
        query = dynamic_cast<KDbQuerySchema *>(window()->schemaObject());
    }

    if (mode != 0/*failure only if it is switching from prev. view*/ && !query) {
        //! @todo msg
        return false;
    }

    if (query) {
        // Use query with Kexi keywords (but not driver-specific keywords) escaped.
        temp->setQuery(query);
        if (temp->queryChangedInView() != Kexi::NoViewMode) {
            KDbSelectStatementOptions options;
            options.setAddVisibleLookupColumns(false);
            KDbNativeStatementBuilder builder(conn, KDb::KDbEscaping);
            if (!builder.generateSelectStatement(&d->origStatement, query, options)) {
                //! @todo msg
                return false;
            }
        }
    }
    if (d->origStatement.isEmpty() && !window()->partItem()->neverSaved()) {
        //no valid query delivered or query has not been modified:
        // just load sql text, no matter if it's valid
        QString sql;
        if (!loadDataBlock(&sql, "sql", true /*canBeEmpty*/)) {
            return false;
        }
        d->origStatement = KDbEscapedString(sql);
        d->slotTextChangedEnabled = false;
        d->editor->setText(d->origStatement.toString());
        d->slotTextChangedEnabled = true;
    }

    if (temp->queryChangedInView() == Kexi::DesignViewMode /* true in this scenario:
                                                      - user switched from SQL to Design,
                                                      - changed the design,
                                                      - switched to Data
                                                      - switched back to SQL */
        || mode != Kexi::DataViewMode) /* true in this scenario: user switched from No-view
                                          or Design view */
    {
        if (!compareSql(d->origStatement.toString(), d->editor->text())) {
            d->slotTextChangedEnabled = false;
            d->editor->setText(d->origStatement.toString());
            d->slotTextChangedEnabled = true;
        }
    }
    QTimer::singleShot(100, d->editor, SLOT(setFocus()));
    return true;
}

QString KexiQueryDesignerSqlView::sqlText() const
{
    return d->editor->text();
}

bool KexiQueryDesignerSqlView::slotCheckQuery()
{
    QString sqlText(d->editor->text().trimmed());
    if (sqlText.isEmpty()) {
        delete d->parsedQuery;
        d->parsedQuery = 0;
        setStatusEmpty();
        return true;
    }

    KDbParser *parser = KexiMainWindowIface::global()->project()->sqlParser();
    const bool ok = parser->parse(KDbEscapedString(sqlText));
    delete d->parsedQuery;
    d->parsedQuery = parser->query();
    if (!d->parsedQuery || !ok || !parser->error().type().isEmpty()) {
        KDbParserError err = parser->error();
        setStatusError(err.message());
        d->editor->jump(err.position());
        delete d->parsedQuery;
        d->parsedQuery = 0;
        return false;
    }

    setStatusOk();
    return true;
}

void KexiQueryDesignerSqlView::slotTextChanged()
{
    if (!d->slotTextChangedEnabled)
        return;
    setDirty(true);
    setStatusEmpty();
}

void KexiQueryDesignerSqlView::updateActions(bool activated)
{
    if (activated) {
        if (isDirty()) {
            slotCheckQuery();
        }
    }
    setAvailable("querypart_check_query", true);
    KexiView::updateActions(activated);
}

KexiQueryPartTempData* KexiQueryDesignerSqlView::tempData() const
{
    return dynamic_cast<KexiQueryPartTempData*>(window()->data());
}

KDbObject* KexiQueryDesignerSqlView::storeNewData(const KDbObject& object,
                                                           KexiView::StoreNewDataOptions options,
                                                           bool *cancel)
{
    Q_ASSERT(cancel);
    Q_UNUSED(options);

    //here: we won't store query layout: it will be recreated 'by hand' in GUI Query Editor
    const bool queryOK = slotCheckQuery();
    bool ok = true;
    KDbObject* query = 0;
    if (queryOK) {
        if (d->parsedQuery) {
            query = d->parsedQuery; //will be returned, so: don't keep it
            d->parsedQuery = 0;
        }
        else { //empty query
            query = new KDbObject(); //just empty
        }
    } else { // the query is not ok
        if (KMessageBox::Yes
            != KMessageBox::questionYesNo(
                   this, xi18n("<para>This query is invalid.</para>"
                               "<para>Do you want to save it?</para>"),
                   0, KStandardGuiItem::save(),
                    KStandardGuiItem::dontSave(),
                   "askBeforeSavingInvalidQueries" /*config entry*/))
        {
            *cancel = true;
            return 0;
        }
        query = new KDbObject(); //just empty
    }

    (KDbObject&)*query = object; //copy main attributes

    ok = KexiMainWindowIface::global()->project()->dbConnection()->storeNewObjectData(query);
    if (ok) {
        ok = KexiMainWindowIface::global()->project()->removeUserDataBlock(query->id()); // for sanity
    }
    if (ok) {
        window()->setId(query->id());
        ok = storeDataBlock(d->editor->text(), "sql");
    }
    if (!ok) {
        delete query;
        query = 0;
    }
    return query;
}

tristate KexiQueryDesignerSqlView::storeData(bool dontAsk)
{
    if (window()->schemaObject()) { //set this instance as obsolete (only if it's stored)
        KexiMainWindowIface::global()->project()->dbConnection()->setQuerySchemaObsolete(window()->schemaObject()->name());
    }
    tristate res = KexiView::storeData(dontAsk);
    if (~res)
        return res;
    if (res == true) {
        res = storeDataBlock(d->editor->text(), "sql");
#if 0
        bool queryOK = slotCheckQuery();
        if (queryOK) {
            res = storeDataBlock(d->editor->text(), "sql");
        } else {
            //query is not ok
            //! @todo allow saving invalid queries
            //! @todo just ask this question:
            res = false;
        }
#endif
    }
    if (res == true) {
        QString empty_xml;
        res = storeDataBlock(empty_xml, "query_layout");   //clear
    }
    if (!res)
        setDirty(true);
    return res;
}
