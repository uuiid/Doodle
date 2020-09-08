#ifndef TEST_FTP_H
#define TEST_FTP_H

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

#endif // TEST_FTP_H
