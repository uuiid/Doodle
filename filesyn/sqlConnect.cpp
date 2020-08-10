#include "sqlConnect.h"
#include <sqlite3.h>
//#include <sqlite3ext.h>

sqlConnect::sqlConnect( )
{
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    rc = sqlite3_open("test.db", &db);
    //sqlite3_open("test.db", &db);
}

sqlConnect::~sqlConnect( )
{
}