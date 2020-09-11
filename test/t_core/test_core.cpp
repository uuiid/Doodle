#include "test_core.h"
#include "src/coreset.h"
#include <QDebug>

test_core::test_core()
{

}

void test_core::init()
{
//    doCore::coreSet& set = doCore::coreSet::getCoreSet();
//    set.init();
}

void test_core::test_set_query()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    set.init();
    qDebug() << set.getCacheRoot().absolutePath();
}

void test_core::test_set_synpath()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    set.init();
    for (doCore::synPath_struct &p:set.getSynDir()){
        qDebug() <<  "\n local -->" <<  p.local << "\n server-->" << p.server;

    }

}
