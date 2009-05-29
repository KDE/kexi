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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */
#ifndef KRSCRIPTHANDLER_H
#define KRSCRIPTHANDLER_H
#include <QObject>
#include <QString>
#include <kexidb/connection.h>
#include <krsectiondata.h>
#include <kross/core/action.h>
#include "krscriptconstants.h"
#include <kdeversion.h>

class KRScriptFunctions;
class KRScriptDebug;
class KRReportData;
class OROPage;
class KRScriptDraw;
class KexiScriptAdaptor;

namespace Scripting{
    class Report;
    class Section;
}
class KRScriptHandler : public QObject
{
    Q_OBJECT
public:
    KRScriptHandler(const KexiDB::Cursor *, KRReportData*);
    ~KRScriptHandler();
    void setSource(const QString &s);
    QVariant evaluate(const QString&);
    void displayErrors();

public slots:

    void slotEnteredSection(KRSectionData*, OROPage*, QPointF);
    void slotEnteredGroup(const QString&, const QVariant&);
    void slotExitedGroup(const QString&, const QVariant&);
    void populateEngineParameters(KexiDB::Cursor *q);
    void setPageNumber(int p) {m_constants->setPageNumber(p);}
    void setPageTotal(int t) {m_constants->setPageTotal(t);}
    void newPage();
    
private:
    KRScriptFunctions *m_functions;
    KRScriptConstants *m_constants;
    KRScriptDebug *m_debug;
    KRScriptDraw *m_draw;
    
    KexiScriptAdaptor *m_kexi;

    Scripting::Report *m_report;

    #if !KDE_IS_VERSION(4,2,88)
    QString fieldFunctions();
    #endif
    
    QString scriptCode();
    
    KexiDB::Connection *m_connection;
    const KexiDB::Cursor *m_cursor;

    QString m_source;
    KRReportData  *m_reportData;

    Kross::Action* m_action;

    QMap<QString, QVariant> m_groups;
    QMap<KRSectionData*, Scripting::Section*> m_sectionMap;
    QString where();
};

#endif

