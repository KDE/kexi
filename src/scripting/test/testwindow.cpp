/***************************************************************************
 * testwindow.cpp
 * This file is part of the KDE project
 * copyright (C)2004-2005 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 ***************************************************************************/

#include "testwindow.h"

#include <qlabel.h>
#include <qvbox.h>
#include <qvgroupbox.h>
//#include <qhgroupbox.h>
#include <qcombobox.h>
#include <ktextedit.h>
#include <kpushbutton.h>

#include <kdebug.h>

TestWindow::TestWindow(const QString& interpretername, const QString& scriptcode)
    : KMainWindow()
    , m_interpretername(interpretername)
    , m_scriptcode(scriptcode)
{
    m_scriptcontainer = Kross::Api::Manager::scriptManager()->getScriptContainer("test");

    QVBox* mainbox = new QVBox(this);

    QVGroupBox* interpretergrpbox = new QVGroupBox("Interpreter", mainbox);
    QStringList interpreters = Kross::Api::Manager::scriptManager()->getInterpreters();
    m_interpretercombo = new QComboBox(interpretergrpbox);
    m_interpretercombo->insertStringList(interpreters);
    m_interpretercombo->setCurrentText(interpretername);

    QVGroupBox* scriptgrpbox = new QVGroupBox("Scripting code", mainbox);
    m_codeedit = new KTextEdit(m_scriptcode, QString::null, scriptgrpbox);
    m_codeedit->setWordWrap(QTextEdit::NoWrap);
    m_codeedit->setTextFormat(Qt::PlainText);

    QHBox* btnbox = new QHBox(mainbox);
    KPushButton* execbtn = new KPushButton("Execute", btnbox);
    connect(execbtn, SIGNAL(clicked()), this, SLOT(execute()));

    setCentralWidget(mainbox);
    setMinimumSize(600,420);
}

TestWindow::~TestWindow()
{
}

void TestWindow::execute()
{
    m_scriptcontainer->setInterpreterName( m_interpretercombo->currentText() );
    m_scriptcontainer->setCode(m_codeedit->text());
    Kross::Api::Object::Ptr result = m_scriptcontainer->execute();
    if(m_scriptcontainer->hadException()) {
        kdDebug() << "EXCEPTION => " << m_scriptcontainer->getException()->toString() << endl;
    }
    else {
        kdDebug() << "DONE => " << result->toString() << endl;
    }
}

#include "testwindow.moc"
