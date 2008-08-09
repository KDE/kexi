/*
* Authors:
*        Julia Sanchez-Simon        <hithwen@gmail.com>
*        Miguel Angel Aragüez-Rey   <fizban87@gmail.com>
*/

#ifdef ORACLEMIGRATE_H
#define NAMESPACE KexiMigration
#else
#define NAMESPACE KexiDB
#endif

#include <qstringlist.h>
#include <qfile.h>
#include <kdebug.h>
#include "oracleconnection_p.h"
#include <kexidb/connectiondata.h>
#include <string>

using namespace NAMESPACE;
using namespace oracle::occi;
using namespace std;

/* ************************************************************************** */
OracleConnectionInternal::OracleConnectionInternal(KexiDB::Connection*
connection)
   : ConnectionInternal(connection)
    , env(0)
    , oraconn(0)
    , errno(0)
    , rs(0)
{
	KexiDBDrvDbg << "OracleConnectionInternal::Constructor: "<< endl;
   try{
      env = Environment::createEnvironment();
   }catch (&ea){
      errno=ea.getErrorCode();
      errmsg=strdup(ea.what());
      KexiDBDrvDbg <<errmsg;
   }	
    
}

OracleConnectionInternal::~OracleConnectionInternal()
{
 KexiDBDrvDbg <<"~OracleConnectionInternal(): ";
 try{
	 	Environment::terminateEnvironment(env);
	 	env=0;
	 	KexiDBDrvDbg <<endl;
	}
	catch (&ea){
      errno=ea.getErrorCode();
      errmsg=strdup(ea.what());
      KexiDBDrvDbg <<errmsg;
  }
}
/* ************************************************************************** */
/*! Connects to the Oracle server on host as the given user using the specified
    password. If the server is on a remote machine, then a port is 
    the port that the remote server is listening on.
 */
/*
* MySQL driver-developers: is latin1() encoding here valid? 
* what about using UTF for passwords?
* Julia: This is to add the C '\0' char at the end, QString 
* (as returned by unicode()) is generally not terminated by '\0'
*/
bool OracleConnectionInternal::db_connect(const KexiDB::ConnectionData& data)
{
	KexiDBDrvDbg<<endl;
  char *port=(char*)malloc(10*sizeof(char));
  sprintf(port,"%d",data.port);
	QString hostName; 
	QString sid="LCC";
	//sid= data.sid.latin1(); //To be included in the API
	
	if (data.hostName.isEmpty() || data.hostName.lower()=="localhost") {
		//localSocketFile not suported
			hostName = "127.0.0.1"; 
	}
	else
	{
		hostName= data.hostName.latin1();
	}
  QString connectStr=("//"+hostName+":"+port+"/"+sid).latin1();
  try{
    oraconn = env->createConnection(data.userName.latin1(),
																		data.password.latin1(),
																		connectStr.latin1());
	  stmt=oraconn->createStatement();
	  return true;
  }
  catch (&ea)
  {
     errno=ea.getErrorCode();
     errmsg=strdup(ea.what());
	   return false;
  }	
}
void OracleConnectionInternal::storeResult(){}
/*! Disconnects from the database.
 */
bool OracleConnectionInternal::db_disconnect()
{
  KexiDBDrvDbg<<endl;
  try{
	  oraconn->terminateStatement(stmt);
	  env->terminateConnection(oraconn);
    oraconn=0;
	  return true;
	  }
	 catch (&ea)
	 {
	  errmsg=ea.getMessage().c_str();
	  KexiDBDrvDbg<<errmsg<<endl;
	  errno=ea.getErrorCode();
	  return false;
	 }
}

/* ************************************************************************** */
/*! Makes no sense in oracle so cheks if dbname is current user
 */
bool OracleConnectionInternal::useDatabase(const QString &dbName) 
{
  KexiDBDrvDbg<<endl;
	QString user; 
	try{
		rs=stmt->executeQuery("SELECT user FROM DUAL");
		if(rs->next()) user=QString(rs->getString(1).c_str());
		stmt->closeResultSet(rs);
		rs=0;
		return !user.compare(dbName);
	}
	catch (&ea)
  {
       errno=ea.getErrorCode();
       errmsg=strdup(ea.what());
       KexiDBDrvDbg <<"OracleConnectionInternal::useDatabase:"<<ea.what()<<endl;
       return(false);
  }
}

bool OracleConnectionInternal::executeSQL(const QString& statement) {
  KexiDBDrvDbg<<statement<<endl;
    const char *query=statement.utf8();
    try
    {
      stmt->execute(query);
      rs=stmt->getResultSet();
      return(true);
    }
    catch (&ea)
    {
       errno=ea.getErrorCode();
       errmsg=strdup(ea.what());
       KexiDBDrvDbg<<errmsg;
       return(false);
    }
}
QString OracleConnectionInternal::escapeIdentifier(const QString& str) const {
	return QString(str).replace('`', "'");
}
QString OracleConnectionInternal::getServerVersion()
{
	try
	{ 
		return QString(oraconn->getServerVersion().c_str());
	}
	catch (&ea)
  {
       errno=ea.getErrorCode();
       errmsg=strdup(ea.what());
       return(NULL);
  }	
}
void OracleConnectionInternal::createSequences(){
  KexiDBDrvDbg<<endl; 
  string sq[5]={"ROW_ID", "BLOBS","OBJECTDATA","OBJECTS","PARTS"};
 
  for(int i=0;i<5;i++)
  {
    try
    {
      //rs=stmt->executeQuery
      //("SELECT 1 FROM USER_OBJECTS WHERE OBJECT_NAME LIKE 'KEXI__SEQ__'"/*+sq[i]+"'"*/);
      //if(!rs->next())
      //{
        KexiDBDrvDbg<<endl; 
        stmt->execute("CREATE SEQUENCE KEXI__SEQ__"+sq[i]);
      //}
    }
    catch (&ea)
	  {
	    KexiDBDrvDbg << ea.what()<< endl;
	  }
  }
}
	
void OracleConnectionInternal::createTriggers()
{
  KexiDBDrvDbg <<endl;
  string tg[4]={"BLOBS","OBJECTDATA","OBJECTS","PARTS"};
  string o[4]={"O","O","O","P"};
  string create="CREATE OR REPLACE TRIGGER KEXI__TG__";
  string before="\nBEFORE INSERT ON KEXI__";
  string begin="\nFOR EACH ROW\nBEGIN\nSELECT KEXI__SEQ__";
  string nextval=".NEXTVAL INTO :NEW.";
  string into="_ID FROM DUAL;\nEND;";
  
  string tgaux="CREATE OR REPLACE TRIGGER KEXI__TG__AUX\n";
	      tgaux+="AFTER INSERT ON KEXI__OBJECTS FOR EACH ROW\nBEGIN\n";
        tgaux+="INSERT INTO KEXI__AUX VALUES (:NEW.ROWID,SYSTIMESTAMP);\nEND;";
  try
  {
    for (int i=0;i<4;i++)
    {
      rs=stmt->executeQuery
	        ("SELECT 1 FROM USER_TABLES WHERE TABLE_NAME LIKE 'KEXI__"+tg[i]+"'");
	    if(rs->next())
	    {
        stmt->execute(create+tg[i]+before+tg[i]+begin+tg[i]+nextval+o[i]+into);
      }
      stmt->closeResultSet(rs);
      rs=0;
    }
    /*rs=stmt->executeQuery
	        ("SELECT 1 FROM USER_TABLES WHERE TABLE_NAME LIKE 'KEXI__PARTS'");
	  if(rs->next())
	  {  
      KexiDBDrvDbg <<"AUX"<<endl;
      KexiDBDrvDbg <<endl<<tgaux.c_str()<<endl;
      //stmt->execute(tgaux);
    }
    */
  }    
	catch (&ea)
	{
	  KexiDBDrvDbg << ea.what()<< endl;
	}
}
//--------------------------------------

OracleCursorData::OracleCursorData(KexiDB::Connection* connection)
: OracleConnectionInternal(connection)
, lengths(0)
, numRows(0)
{}

OracleCursorData::~OracleCursorData(){}

