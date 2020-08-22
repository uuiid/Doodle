#include <gtest/gtest.h>
#include "filesyn/sqlConnect.h"
#include "filesyn/fileInfo.h"

TEST(test_sqlComm, sqlComm)
{
    sqlConnect& sql = sqlConnect::GetSqlCommect( );
    EXPECT_TRUE(sql.openSqlDB( ));
}

TEST(test_sqlCom_sub,sqlcom_sub)
{
    char* createTable = "CREATE TABLE IF NOT EXISTS @firstTable (\
    id integer primary key AUTOINCREMENT,\
    file_size integer,\
    modify_time text,\
    path text\
    )";
    sqlConnect& sql = sqlConnect::GetSqlCommect( );
    sql.openSqlDB( );
    sqlite3_stmt * stmt;
    sqlite3_prepare(sql.getDB(),createTable,-1,&stmt,NULL);
    sqlite3_bind_text(stmt,1,"test",-1,NULL);
    int err = sqlite3_step(stmt);
    if (err != SQLITE_OK){
        std::cout << err << std::endl;
    }
    sqlite3_step(stmt);
    //stmt = fileInfo::getCreateTable(,stmt,"test");
    EXPECT_TRUE(sql.exeStmtNoReturn(stmt));
}