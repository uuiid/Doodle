#pragma once


#include <QObject>

class test_ftp:public QObject
{
    Q_OBJECT
public:
    test_ftp();
private slots:
    void test_upload();
    void test_down();
    void test_getInfo();
    void test_getList();

};
