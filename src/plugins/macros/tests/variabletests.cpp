/***************************************************************************
 * This file is part of the KDE project
 * copyright (C) 2006 by Sebastian Sauer (mail@dipe.org)
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "variabletests.h"
#include "testobject.h"
#include "testaction.h"
#include "komacrotestbase.h"

#include "../lib/action.h"
#include "../lib/function.h"
#include "../lib/manager.h"
#include "../lib/macro.h"
#include "../lib/variable.h"
#include "../lib/metaobject.h"
#include "../lib/metamethod.h"
#include "../lib/metaparameter.h"
#include "../lib/exception.h"
#include "../lib/macroitem.h"

#include <ostream>

#include <qstringlist.h>
#include <qdom.h>

#include <kdebug.h>
#include <kunittest/runner.h>
#include <kxmlguiclient.h>

using namespace KUnitTest;
using namespace KoMacroTest;

namespace KoMacroTest {

	/**
	* Register KoMacroTest::CommonTests as TestSuite.
	*/
	
	KUNITTEST_SUITE("KoMacroTestSuite");
	KUNITTEST_REGISTER_TESTER(VariableTests);
	

	class VariableTests::Private
	{
		public:		
		/**
		* An KXMLGUIClient instance created on @a setUp() and
		* passed to the @a KoMacro::Manager to bridge to the
		* app-functionality.
		*/
		KXMLGUIClient* xmlguiclient;

		/**
		* An @a TestObject instance used internaly to test
		* handling and communication with from QObject
		* inheritated instances.
		*/
		TestAction* testaction;

		QDomDocument* doomdocument;
		
		KSharedPtr<KoMacro::Macro> macro;

		Private()
			: xmlguiclient(0)
			, testaction(0)
			, doomdocument(0)
			, macro(0)
		{
		}
	};
}

typedef QValueList< KSharedPtr<KoMacro::MacroItem> >::size_type sizetype;

/******************************************************************************
* This is an xtra big TODO: 
* - get rid of all double declarations
* - create xtra-class for Variable/Macroitem tests 
* - add comments
******************************************************************************/
VariableTests::VariableTests()
	: KUnitTest::SlotTester()
	, d( new Private() ) // create the private d-pointer instance.
{
}

VariableTests::~VariableTests()
{
	delete d->xmlguiclient;
	delete d;
}


void VariableTests::setUp()
{
	d->xmlguiclient = new KXMLGUIClient();
	
	if (::KoMacro::Manager::self() == 0) {	
		::KoMacro::Manager::init( d->xmlguiclient );
	}
	
	d->testaction = new TestAction();
	::KoMacro::Manager::self()->publishAction(d->testaction);	

	d->doomdocument = new QDomDocument();

	QString const xml = QString("<!DOCTYPE macros>"
				   "<macro xmlversion=\"1\" >"
				      "<item action=\"testaction\" >"
				      "</item>"
				    "</macro>");
	
	d->doomdocument->setContent(xml);
	d->macro = KoMacro::Manager::self()->createMacro("testMacro");
	d->macro->parseXML(d->doomdocument->documentElement());
	d->macro->execute(this);
}

void VariableTests::tearDown()
{
	delete d->macro;
	delete d->doomdocument;
	delete d->xmlguiclient;
}

void VariableTests::testMacro()
{
	kdDebug()<<"===================== testVariable() ===================" << endl;
	kdDebug()<<"===================== testMacro() ======================" << endl;
	
	//fetch Items and ..
	QValueList< KSharedPtr<KoMacro::MacroItem> >& items = d->macro->items();

	//... check that there is one
	KOMACROTEST_XASSERT( items.count(), sizetype(0) );
}

void VariableTests::testVariableString() {
	kdDebug()<<"===================== testVariableString() ======================" << endl;			
	QValueList< KSharedPtr<KoMacro::MacroItem> >& items = d->macro->items();	
	KSharedPtr<KoMacro::Action> actionptr = items[0]->action();
	
	//fetch the "teststring"-variable
	KSharedPtr<KoMacro::Variable> variableptr = actionptr->variable(TESTSTRING);	
	//So there is a variable, does hasVariable() work ?
	KOMACROTEST_ASSERT(actionptr->hasVariable(TESTSTRING),true);
	//check count of variables
	KOMACROTEST_ASSERT(sizetype(actionptr->variableNames().count()),sizetype(4));
	//remove one
	actionptr->removeVariable(TESTSTRING);
	//Decreased ??
	KOMACROTEST_ASSERT(sizetype(actionptr->variableNames().count()),sizetype(3));	
	//add one
	actionptr->setVariable(variableptr);
	//increased ??
	KOMACROTEST_ASSERT(sizetype(actionptr->variableNames().count()),sizetype(4));	
	
	//check that it is not null
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	//check that it is "testString"
	KOMACROTEST_ASSERT(variableptr->variant().toString(),QString("testString"));

	actionptr->setVariable("teststring", "STRINGTEST", "TestString");
	variableptr = actionptr->variable("teststring");
	//check that it is not null
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	//check that it is " "
	KOMACROTEST_ASSERT(variableptr->variant().toString(),QString("TestString"));
}

void VariableTests::testVariableInt() {
	kdDebug()<<"===================== testVariableInt() ======================" << endl;			
	QValueList< KSharedPtr<KoMacro::MacroItem> >& items = d->macro->items();	
	KSharedPtr<KoMacro::Action> actionptr = items[0]->action();
	
	//fetch the "testint"-variable
	KSharedPtr<KoMacro::Variable> variableptr = actionptr->variable(TESTINT);	
	//check that it is not null
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	//check that it is 0
	KOMACROTEST_ASSERT(variableptr->variant().toInt(),int(0));

	actionptr->setVariable(TESTINT,"INTTEST",INT_MAX);
	variableptr = actionptr->variable("testint");
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	KOMACROTEST_ASSERT(sizetype(variableptr->variant().toInt()),sizetype(INT_MAX));

	actionptr->setVariable(TESTINT,"INTTEST",INT_MAX+1);
	variableptr = actionptr->variable("testint");
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	KOMACROTEST_ASSERT(sizetype(variableptr->variant().toInt()),sizetype(INT_MAX+1));

	actionptr->setVariable(TESTINT,"INTTEST",INT_MIN);
	variableptr = actionptr->variable("testint");
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	KOMACROTEST_ASSERT(sizetype(variableptr->variant().toInt()),sizetype(INT_MIN));

	actionptr->setVariable(TESTINT,"INTTEST",INT_MIN-1);
	variableptr = actionptr->variable("testint");
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	KOMACROTEST_ASSERT(sizetype(variableptr->variant().toInt()),sizetype(INT_MIN-1));
}

void VariableTests::testVariableBool() {
	kdDebug()<<"===================== testVariableBool() ======================" << endl;			
	QValueList< KSharedPtr<KoMacro::MacroItem> >& items = d->macro->items();	
	KSharedPtr<KoMacro::Action> actionptr = items[0]->action();	
	
	//fetch the "testbool"-variable
	KSharedPtr<KoMacro::Variable> variableptr = actionptr->variable(TESTBOOL);	
	//check that it is not null
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	//check that it is " "
	KOMACROTEST_ASSERT(variableptr->variant().toBool(),true);

	actionptr->setVariable("testbool","BOOLTEST", "false");
	variableptr = actionptr->variable("testbool");
	KOMACROTEST_XASSERT(sizetype(variableptr.data()), sizetype(0));
	KOMACROTEST_ASSERT(variableptr->variant().toBool(),false);	
}
#include "variabletests.moc"
