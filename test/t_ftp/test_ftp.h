#ifndef TEST_FTP_H
#define TEST_FTP_H

#include <QObject>

class test_ftp:public QObject
{
    Q_OBJECT
public:
    test_ftp();
private slots:
    void test_d();
    void test_down();
};

#endif // TEST_FTP_H
