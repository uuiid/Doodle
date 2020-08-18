#include "sqlConnect.h"
#include <sqlite3.h>

#include <iostream>
#include <exception>

//#include <sqlite3ext.h>

sqlConnect::sqlConnect( )
{
    db = nullptr;
    ZErrMsg = "ok";
}




sqlConnect::~sqlConnect( )
{
    sqlite3_close(db);
    delete ZErrMsg;
}

char* sqlConnect::openSqlDB( )
{
    int rc;
    rc = sqlite3_open(".doodle_syn.db", &db);
    if (rc)
    {
        throw std::runtime_error("无法打开数据库");
    }
    return "fail";
}

void sqlConnect::subObj( )
{
}
