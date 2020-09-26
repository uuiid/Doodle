#include "test_filesyn.h"
#include "src/sqlconnect.h"
#include "src/fileinfo.h"
#include "src/foldercompare.h"
test_fileSyn::test_fileSyn()
{

}

void test_fileSyn::init()
{
    qDebug()<<"state test";doFileSyn::sqlConnect& taa = doFileSyn::sqlConnect::GetSqlConnect();
    taa.openSqlDB(QDir("D:/tmp"));
    doFileSyn::d_setting& s = doFileSyn::d_setting::GetSetting();
    s.setBackup(QUrl::fromLocalFile("D:/tmp2"));
    s.fastSetSynDef_syn();
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

void test_fileSyn::test_fileIno_sql_sub()
{
    doFileSyn::fileInfo t("D:/sc_064","D:/tmp","BuJu.1001.png");
    QVERIFY(t.subAndUpdataSQL());
}

void test_fileSyn::test_fileInfo_sql_delete()
{
    doFileSyn::fileInfo t("D:/sc_064","D:/tmp","BuJu.1001.png");
    QVERIFY(t.SQLdelete());
}

void test_fileSyn::test_fileInfo_filsyn()
{
    doFileSyn::fileInfo t("D:/sc_064","D:/tmp","BuJu.1002.png");
    t.com();
    QVERIFY(t.syn());
}
