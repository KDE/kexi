/* This file is part of the KDE project
   Copyright (C) 2004, 2006 Jaroslaw Staniek <js@iidea.pl>

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

#include "parser_p.h"
#include "sqlparser.h"

#include <kdebug.h>
#include <klocale.h>

#include <qregexp.h>

#include <assert.h>

using namespace KexiDB;

Parser *parser = 0;
Field *field = 0;
//bool requiresTable;
QPtrList<Field> fieldList;
int current = 0;
QString ctoken = "";

//-------------------------------------

ParserPrivate::ParserPrivate()
 : reservedKeywords(997, 997, false)
 , initialized(false)
{
	clear();
	table = 0;
	select = 0;
	db = 0;
}

ParserPrivate::~ParserPrivate()
{
	delete select;
	delete table;
}

void ParserPrivate::clear()
{
	operation = Parser::OP_None;
	error = ParserError();
}

//-------------------------------------

ParseInfo::ParseInfo(KexiDB::QuerySchema *query)
 : repeatedTablesAndAliases(997, false)
 , querySchema(query)
{
	repeatedTablesAndAliases.setAutoDelete(true);
}

ParseInfo::~ParseInfo()
{
}

//-------------------------------------

extern int yyparse();
extern void tokenize(const char *data);

void yyerror(const char *str)
{
	KexiDBDbg << "error: " << str << endl;
	KexiDBDbg << "at character " << current << " near tooken " << ctoken << endl;
	parser->setOperation(Parser::OP_Error);

	const bool otherError = (qstrnicmp(str, "other error", 11)==0);
	
	if (parser->error().type().isEmpty() 
		&& (str==0 || strlen(str)==0 
		|| qstrnicmp(str, "syntax error", 12)==0 || qstrnicmp(str, "parse error", 11)==0)
		|| otherError)
	{
		KexiDBDbg << parser->statement() << endl;
		QString ptrline = "";
		for(int i=0; i < current; i++)
			ptrline += " ";

		ptrline += "^";

		KexiDBDbg << ptrline << endl;

		//lexer may add error messages
		QString lexerErr = parser->error().error();

		QString errtypestr(str);
		if (lexerErr.isEmpty()) {
#if 0
			if (errtypestr.startsWith("parse error, unexpected ")) {
				//something like "parse error, unexpected IDENTIFIER, expecting ',' or ')'"
				QString e = errtypestr.mid(24);
				KexiDBDbg << e <<endl;
				QString token = "IDENTIFIER";
				if (e.startsWith(token)) {
					QRegExp re("'.'");
					int pos=0;
					pos = re.search(e, pos);
					QStringList captured=re.capturedTexts();
					if (captured.count()>=2) {
//						KexiDBDbg << "**" << captured.at(1) << endl;
//						KexiDBDbg << "**" << captured.at(2) << endl;
					}
				}
					
					
					
//			 IDENTIFIER, expecting '")) {
				e = errtypestr.mid(47);
				KexiDBDbg << e <<endl;
//				,' or ')'
//		lexerErr i18n("identifier was expected");
				
			} else 
#endif
			if (errtypestr.startsWith("parse error, expecting `IDENTIFIER'"))
				lexerErr = i18n("identifier was expected");
		}
		
		if (!otherError) {
			if (!lexerErr.isEmpty())
				lexerErr.prepend(": ");

			if (parser->isReservedKeyword(ctoken.latin1()))
				parser->setError( ParserError(i18n("Syntax Error"), 
					i18n("\"%1\" is a reserved keyword").arg(ctoken)+lexerErr, ctoken, current) );
			else
				parser->setError( ParserError(i18n("Syntax Error"), 
					i18n("Syntax Error near \"%1\"").arg(ctoken)+lexerErr, ctoken, current) );
		}
	}
}

void setError(const QString& errName, const QString& errDesc)
{
	parser->setError( ParserError(errName, errDesc, ctoken, current) );
	yyerror(errName.latin1());
}

void setError(const QString& errDesc)
{
	setError("other error", errDesc);
}

/* this is better than assert() */
#define IMPL_ERROR(errmsg) setError("Implementation error", errmsg)

bool parseData(Parser *p, const char *data)
{
/* todo: make this REENTRANT */
	parser = p;
	parser->clear();
	field = 0;
	fieldList.clear();
//	requiresTable = false;

	if (!data) {
		ParserError err(i18n("Error"), i18n("No query specified"), ctoken, current);
		parser->setError(err);
		yyerror("");
		parser = 0;
		return false;
	}

	tokenize(data);
	if (!parser->error().type().isEmpty()) {
		parser = 0;
		return false;
	}
	yyparse();

	bool ok = true;
	if(parser->operation() == Parser::OP_Select)
	{
		KexiDBDbg << "parseData(): ok" << endl;
//			KexiDBDbg << "parseData(): " << tableDict.count() << " loaded tables" << endl;
/*			TableSchema *ts;
			for(QDictIterator<TableSchema> it(tableDict); TableSchema *s = tableList.first(); s; s = tableList.next())
			{
				KexiDBDbg << "  " << s->name() << endl;
			}*/
/*removed
			Field::ListIterator it = parser->select()->fieldsIterator();
			for(Field *item; (item = it.current()); ++it)
			{
				if(tableList.findRef(item->table()) == -1)
				{
					ParserError err(i18n("Field List Error"), i18n("Unknown table '%1' in field list").arg(item->table()->name()), ctoken, current);
					parser->setError(err);

					yyerror("fieldlisterror");
					ok = false;
				}
			}*/
			//take the dummy table out of the query
//			parser->select()->removeTable(dummy);
	}
	else {
		ok = false;
	}

//		tableDict.clear();
	parser = 0;
	return ok;
}

	
/* Adds \a column to \a querySchema. \a column can be in a form of
 table.field, tableAlias.field or field
*/
bool addColumn( ParseInfo& parseInfo, BaseExpr* columnExpr )
{
	if (!columnExpr->validate(parseInfo)) {
		setError(parseInfo.errMsg, parseInfo.errDescr);
		return false;
	}

	VariableExpr *v_e = columnExpr->toVariable();
	if (columnExpr->exprClass() == KexiDBExpr_Variable && v_e) {
		//it's a variable:
		if (v_e->name=="*") {//all tables asterisk
			if (parseInfo.querySchema->tables()->isEmpty()) {
				setError(i18n("\"*\" could not be used if no tables are specified"));
				return false;
			}
			parseInfo.querySchema->addAsterisk( new QueryAsterisk(parseInfo.querySchema) );
		}
		else if (v_e->tableForQueryAsterisk) {//one-table asterisk
			parseInfo.querySchema->addAsterisk( 
				new QueryAsterisk(parseInfo.querySchema, v_e->tableForQueryAsterisk) );
		}
		else if (v_e->field) {//"table.field" or "field" (bound to a table or not)
			parseInfo.querySchema->addField(v_e->field, v_e->tablePositionForField);
		}
		else {
			IMPL_ERROR("addColumn(): unknown case!");
			return false;
		}
		return true;
	}

	//it's complex expression
	parseInfo.querySchema->addExpression(columnExpr);

#if 0
	KexiDBDbg << "found variable name: " << varName << endl;
	int dotPos = varName.find('.');
	QString tableName, fieldName;
//TODO: shall we also support db name?
	if (dotPos>0) {
		tableName = varName.left(dotPos);
		fieldName = varName.mid(dotPos+1);
	}
	if (tableName.isEmpty()) {//fieldname only
		fieldName = varName;
		if (fieldName=="*") {
			parseInfo.querySchema->addAsterisk( new QueryAsterisk(parseInfo.querySchema) );
		}
		else {
			//find first table that has this field
			Field *firstField = 0;
			for (TableSchema::ListIterator it(*parseInfo.querySchema->tables()); it.current(); ++it) {
				Field *f = it.current()->field(fieldName);
				if (f) {
					if (!firstField) {
						firstField = f;
					} else if (f->table()!=firstField->table()) {
						//ambiguous field name
						setError(i18n("Ambiguous field name"), 
							i18n("Both table \"%1\" and \"%2\" have defined \"%3\" field. "
								"Use \"<tableName>.%4\" notation to specify table name.")
								.arg(firstField->table()->name()).arg(f->table()->name())
								.arg(fieldName).arg(fieldName));
						return false;
					}
				}
			}
			if (!firstField) {
					setError(i18n("Field not found"), 
						i18n("Table containing \"%1\" field not found").arg(fieldName));
					return false;
			}
			//ok
			parseInfo.querySchema->addField(firstField);
		}
	}
	else {//table.fieldname or tableAlias.fieldname
		tableName = tableName.lower();
		TableSchema *ts = parseInfo.querySchema->table( tableName );
		if (ts) {//table.fieldname
			//check if "table" is covered by an alias
			const QValueList<int> tPositions = parseInfo.querySchema->tablePositions(tableName);
			QValueList<int>::ConstIterator it = tPositions.constBegin();
			QCString tableAlias;
			bool covered = true;
			for (; it!=tPositions.constEnd() && covered; ++it) {
				tableAlias = parseInfo.querySchema->tableAlias(*it);
				if (tableAlias.isEmpty() || tableAlias.lower()==tableName.latin1())
					covered = false; //uncovered
				KexiDBDbg << " --" << "covered by " << tableAlias << " alias" << endl;
			}
			if (covered) {
				setError(i18n("Could not access the table directly using its name"), 
					i18n("Table \"%1\" is covered by aliases. Instead of \"%2\", "
					"you can write \"%3\"").arg(tableName)
					.arg(tableName+"."+fieldName).arg(tableAlias+"."+fieldName.latin1()));
				return false;
			}
		}
		
		int tablePosition = -1;
		if (!ts) {//try to find tableAlias
			tablePosition = parseInfo.querySchema->tablePositionForAlias( tableName.latin1() );
			if (tablePosition>=0) {
				ts = parseInfo.querySchema->tables()->at(tablePosition);
				if (ts) {
//					KexiDBDbg << " --it's a tableAlias.name" << endl;
				}
			}
		}


		if (ts) {
			QValueList<int> *positionsList = repeatedTablesAndAliases[ tableName ];
			if (!positionsList) {
				IMPL_ERROR(tableName + "." + fieldName + ", !positionsList ");
				return false;
			}

			if (fieldName=="*") {
				if (positionsList->count()>1) {
					setError(i18n("Ambiguous \"%1.*\" expression").arg(tableName),
						i18n("More than one \"%1\" table or alias defined").arg(tableName));
					return false;
				}
				parseInfo.querySchema->addAsterisk( new QueryAsterisk(parseInfo.querySchema, ts) );
			}
			else {
//				KexiDBDbg << " --it's a table.name" << endl;
				Field *realField = ts->field(fieldName);
				if (realField) {
					// check if table or alias is used twice and both have the same column
					// (so the column is ambiguous)
					int numberOfTheSameFields = 0;
					for (QValueList<int>::iterator it = positionsList->begin();
						it!=positionsList->end();++it)
					{
						TableSchema *otherTS = parseInfo.querySchema->tables()->at(*it);
						if (otherTS->field(fieldName))
							numberOfTheSameFields++;
						if (numberOfTheSameFields>1) {
							setError(i18n("Ambiguous \"%1.%2\" expression").arg(tableName).arg(fieldName),
								i18n("More than one \"%1\" table or alias defined containing \"%2\" field")
								.arg(tableName).arg(fieldName));
							return false;
						}
					}

					parseInfo.querySchema->addField(realField, tablePosition);
				}
				else {
					setError(i18n("Field not found"), i18n("Table \"%1\" has no \"%2\" field")
						.arg(tableName).arg(fieldName));
					return false;
				}
			}
		}
		else {
			tableNotFoundError(tableName);
			return false;
		}
	}
#endif
	return true;
}

//! clean up no longer needed temporary objects
#define CLEANUP \
	delete colViews; \
	delete tablesList; \
	delete options

QuerySchema* buildSelectQuery( 
	QuerySchema* querySchema, NArgExpr* colViews, NArgExpr* tablesList, 
	SelectOptionsInternal* options )
{
	ParseInfo parseInfo(querySchema);
	
	//-------tables list
//	assert( tablesList ); //&& tablesList->exprClass() == KexiDBExpr_TableList );

	uint columnNum = 0;
/*TODO: use this later if there are columns that use database fields, 
        e.g. "SELECT 1 from table1 t, table2 t") is ok however. */
	//used to collect information about first repeated table name or alias:
//	QDict<char> tableNamesAndTableAliases(997, false);
//	QString repeatedTableNameOrTableAlias;
	if (tablesList) {
		for (int i=0; i<tablesList->args(); i++, columnNum++) {
			BaseExpr *e = tablesList->arg(i);
			VariableExpr* t_e = 0;
			QCString aliasString;
			if (e->exprClass() == KexiDBExpr_SpecialBinary) {
				BinaryExpr* t_with_alias = e->toBinary();
				assert(t_with_alias);
				assert(t_with_alias->left()->exprClass() == KexiDBExpr_Variable);
				assert(t_with_alias->right()->exprClass() == KexiDBExpr_Variable
					&& (t_with_alias->token()==AS || t_with_alias->token()==0));
				t_e = t_with_alias->left()->toVariable();
				aliasString = t_with_alias->right()->toVariable()->name.latin1();
			}
			else {
				t_e = e->toVariable();
			}
			assert(t_e);
			QCString tname = t_e->name.latin1();
			TableSchema *s = parser->db()->tableSchema(tname);
			if(!s) {
				setError(//i18n("Field List Error"), 
					i18n("Table \"%1\" does not exist").arg(tname));
	//			yyerror("fieldlisterror");
				CLEANUP;
				return 0;
			}
			QCString tableOrAliasName;
			if (!aliasString.isEmpty()) {
				tableOrAliasName = aliasString;
//				KexiDBDbg << "- add alias for table: " << aliasString << endl;
			} else {
				tableOrAliasName = tname;
			}
			// 1. collect information about first repeated table name or alias
			//    (potential ambiguity)
			QValueList<int> *list = parseInfo.repeatedTablesAndAliases[tableOrAliasName];
			if (list) {
				//another table/alias with the same name
				list->append( i );
//				KexiDBDbg << "- another table/alias with name: " << tableOrAliasName << endl;
			}
			else {
				list = new QValueList<int>();
				list->append( i );
				parseInfo.repeatedTablesAndAliases.insert( tableOrAliasName, list );
//				KexiDBDbg << "- first table/alias with name: " << tableOrAliasName << endl;
			}
	/*		if (repeatedTableNameOrTableAlias.isEmpty()) {
				if (tableNamesAndTableAliases[tname])
					repeatedTableNameOrTableAlias=tname;
				else
					tableNamesAndTableAliases.insert(tname, (const char*)1);
			}
			if (!aliasString.isEmpty()) {
				KexiDBDbg << "- add alias for table: " << aliasString << endl;
	//			querySchema->setTableAlias(columnNum, aliasString);
				//2. collect information about first repeated table name or alias
				//   (potential ambiguity)
				if (repeatedTableNameOrTableAlias.isEmpty()) {
					if (tableNamesAndTableAliases[aliasString])
						repeatedTableNameOrTableAlias=aliasString;
					else
						tableNamesAndTableAliases.insert(aliasString, (const char*)1);
				}
			}*/
//			KexiDBDbg << "addTable: " << tname << endl;
			querySchema->addTable( s, aliasString );
		}
	}

	/* set parent table if there's only one */
//	if (parser->select()->tables()->count()==1)
	if (querySchema->tables()->count()==1)
		querySchema->setMasterTable(querySchema->tables()->first());

	//-------add fields
	if (colViews) {
		BaseExpr *e;
		columnNum = 0;
		const uint colCount = colViews->list.count();
//		for (BaseExpr::ListIterator it(colViews->list);(e = it.current()); columnNum++)
		colViews->list.first();
		for (; columnNum<colCount; columnNum++) {
			e = colViews->list.current();
			bool moveNext = true; //used to avoid ++it when an item is taken from the list
			BaseExpr *columnExpr = e;
			VariableExpr* aliasVariable = 0;
			if (e->exprClass() == KexiDBExpr_SpecialBinary && e->toBinary()
				&& (e->token()==AS || e->token()==0))
			{
				//KexiDBExpr_SpecialBinary: with alias
				columnExpr = e->toBinary()->left();
	//			isFieldWithAlias = true;
				aliasVariable = e->toBinary()->right()->toVariable();
				if (!aliasVariable) {
					setError(i18n("Invalid alias definition for column \"%1\"")
						.arg(columnExpr->toString())); //ok?
					CLEANUP;
					return 0;
				}
			}
	
			const int c = columnExpr->exprClass();
			const bool isExpressionField = 
					c == KexiDBExpr_Const
				|| c == KexiDBExpr_Unary
				|| c == KexiDBExpr_Arithm
				|| c == KexiDBExpr_Logical
				|| c == KexiDBExpr_Relational
				|| c == KexiDBExpr_Const
				|| c == KexiDBExpr_Function
				|| c == KexiDBExpr_Aggregation;
	
			if (c == KexiDBExpr_Variable) {
				//just a variable, do nothing, addColumn() will handle this
			}
			else if (isExpressionField) {
				//expression object will be reused, take, will be owned, do not destroy
//		KexiDBDbg << colViews->list.count() << " " << it.current()->debugString() << endl;
				colViews->list.take(); //take() doesn't work
				moveNext = false;
			}
			else if (aliasVariable) {
				//take first (left) argument of the special binary expr, will be owned, do not destroy
				e->toBinary()->m_larg = 0;
			}
			else {
				setError(i18n("Invalid \"%1\" column definition").arg(e->toString())); //ok?
				CLEANUP;
				return 0;
			}
	
			if (!addColumn( parseInfo, columnExpr )) {
				CLEANUP;
				return 0;
			}
			
			if (aliasVariable) {
//				KexiDBDbg << "ALIAS \"" << aliasVariable->name << "\" set for column " 
//					<< columnNum << endl;
				querySchema->setColumnAlias(columnNum, aliasVariable->name.latin1());
			}
	/*		if (e->exprClass() == KexiDBExpr_SpecialBinary && dynamic_cast<BinaryExpr*>(e)
				&& (e->type()==AS || e->type()==0))
			{
				//also add alias
				VariableExpr* aliasVariable =
					dynamic_cast<VariableExpr*>(dynamic_cast<BinaryExpr*>(e)->right());
				if (!aliasVariable) {
					setError(i18n("Invalid column alias definition")); //ok?
					return 0;
				}
				kdDebug() << "ALIAS \"" << aliasVariable->name << "\" set for column " 
					<< columnNum << endl;
				querySchema->setColumnAlias(columnNum, aliasVariable->name.latin1());
			}*/
	
			if (moveNext) {
				colViews->list.next();
//				++it;
			}
		}
	}
	//----- SELECT options
	if (options) {
		//----- WHERE expr.
		if (options->whereExpr) {
			if (!options->whereExpr->validate(parseInfo)) {
				setError(parseInfo.errMsg, parseInfo.errDescr);
				CLEANUP;
				return false;
			}
			querySchema->setWhereExpression(options->whereExpr);
		}
		//----- ORDER BY
		if (options->orderByColumns) {
			OrderByColumnList &orderByColumnList = querySchema->orderByColumnList();
			OrderByColumnInternal::ListConstIterator it = options->orderByColumns->constEnd();
			uint count = options->orderByColumns->count();
			--it;
			for (;count>0; --it, --count) 
				/*opposite direction due to parser specifics*/
			{
				//first, try to find a column name or alias (outside of asterisks)
				QueryColumnInfo *columnInfo = querySchema->columnInfo( (*it).aliasOrName, false/*outside of asterisks*/ );
				if (columnInfo) {
					orderByColumnList.appendColumn( *columnInfo, (*it).ascending );
				}
				else {
					//failed, try to find a field name within all the tables
					if ((*it).columnNumber != -1) {
						if (!orderByColumnList.appendColumn( *querySchema,
							(*it).ascending, (*it).columnNumber-1 ))
						{
							setError(i18n("Could not define sorting - no column at position %1")
								.arg((*it).columnNumber));
							CLEANUP;
							return 0;
						}
					}
					else {
						Field * f = querySchema->findTableField((*it).aliasOrName);
						if (!f) {
							setError(i18n("Could not define sorting - "
								"column name or alias \"%1\" does not exist").arg((*it).aliasOrName));
							CLEANUP;
							return 0;
						}
						orderByColumnList.appendField( *f, (*it).ascending );
					}
				}
			}
		}
	}

//	KexiDBDbg << "Select ColViews=" << (colViews ? colViews->debugString() : QString::null)
//		<< " Tables=" << (tablesList ? tablesList->debugString() : QString::null) << endl;
	
	CLEANUP;
	return querySchema;
}

#undef CLEANUP

