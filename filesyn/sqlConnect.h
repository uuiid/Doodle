#pragma once

#include <sqlite3.h>
//#include <sqlite3ext.h>

class sqlConnect
{
public:
	sqlConnect( );
	~sqlConnect( );

	char* openSqlDB( );
	void subObj( );



private:
	sqlite3* db;
	char* ZErrMsg;
};

