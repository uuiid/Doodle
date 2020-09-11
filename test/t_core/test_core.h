#ifndef TEST_CORE_H
#define TEST_CORE_H

#include <QObject>

class test_core :public QObject
{
    Q_OBJECT
public:
    test_core();
private slots:
    void init();
    void test_set_query();
    void test_set_synpath();
};

#endif // TEST_CORE_H
