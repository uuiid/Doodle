#pragma once

#include <sqlite3.h>
//#include <sqlite3ext.h>

class sqlConnect
{
public:
	sqlConnect( );
	~sqlConnect( );



private:
	sqlite3* db;
	char* ZErrMsg;
};

