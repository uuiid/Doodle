#include "test_filesyn.h"
#include "src/sqlconnect.h"
#include "src/fileinfo.h"
#include "src/foldercompare.h"
test_fileSyn::test_fileSyn()
{

}

void test_fileSyn::test_sqlconn()
{
    doFileSyn::sqlConnect& taa = doFileSyn::sqlConnect::GetSqlConnect();
    QVERIFY(taa.openSqlDB(QDir("D:/tmp")));
    taa.closedb();
}

void test_fileSyn::test_sqlCreateTable()
{
    doFileSyn::sqlConnect& taa = doFileSyn::sqlConnect::GetSqlConnect();
    if (!taa.openSqlDB(QDir("D:/tmp"))) return ;
    QVERIFY(taa.createTable(doFileSyn::fileInfo::getCreataTableCom("file")));
}

void test_fileSyn::test_scan()
{
    doFileSyn::fileInfo t("D:/sc_064","D:/tmp","BuJu.1001.png");
    QVERIFY(t.subAndUpdataSQL());
}
