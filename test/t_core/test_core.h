#pragma once


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

    void test_create_eps();
};

