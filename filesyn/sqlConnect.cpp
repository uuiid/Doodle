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

bool sqlConnect::openSqlDB( )
{
    int rc;
    if (db != nullptr)
    {
        rc = sqlite3_open("D:\\.doodle_syn.db", &db);
        if (rc)
        {
            throw std::runtime_error("无法打开数据库");
            return false;
        }
    }
    
    return true;
}

void sqlConnect::subObj( )
{
}

bool sqlConnect::exeStmtNoReturn(sqlite3_stmt * stmt )
{
    sqlite3_step(stmt);
    return true;
}

sqlite3* sqlConnect::getDB( )
{
    return db;
}

sqlConnect& sqlConnect::GetSqlCommect( )
{
    static sqlConnect instance;
    return instance;
}
