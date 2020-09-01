#ifndef TEST_FILESYN_H
#define TEST_FILESYN_H

//#include "fileSyn/fileSyn_global.h"
//#include "fileSyn/src/sqlconnect.h"
#include <QtTest/QtTest>
#include "fileSyn_global.h"

class test_fileSyn :public QObject
{
    Q_OBJECT
public:
    test_fileSyn();
private slots:
    void init();
    void test_sqlconn();
    void test_sqlCreateTable();
    void test_fileIno_sql_sub();
    void test_fileInfo_sql_delete();
    void test_fileInfo_filsyn();
};

#endif // TEST_FILESYN_H
