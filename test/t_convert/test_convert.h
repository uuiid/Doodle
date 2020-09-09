#ifndef TEST_CONVERT_H
#define TEST_CONVERT_H

#include <QObject>


class test_convert: public QObject
{
    Q_OBJECT

public:
    test_convert();
private slots:
    void test_conver();
};



#endif //TEST_CONVERT_H
