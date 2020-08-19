#include <gtest/gtest.h>
#include "filesyn/sqlConnect.h"

TEST(test_sqlComm, sqlComm)
{
    sqlConnect& sql = sqlConnect::GetSqlCommect( );
    sql.openSqlDB( );
}