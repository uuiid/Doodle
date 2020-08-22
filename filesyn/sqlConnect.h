#pragma once

#include "fileInfo.h"
#include <sqlite3.h>
//#include <qsql.h>

class sqlConnect
{
public:
	~sqlConnect( );

	bool openSqlDB( );
	void subObj( );

	bool exeStmtNoReturn(sqlite3_stmt* stmt);
	sqlite3* getDB( );
	static sqlConnect& GetSqlCommect( );

private:
	sqlConnect( );
	sqlConnect(const sqlConnect&) = delete;
	sqlConnect& operator = (const sqlConnect& s) = delete;

private:
	sqlite3* db;
	char* ZErrMsg;
};

