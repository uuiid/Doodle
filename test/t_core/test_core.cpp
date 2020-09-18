#include "test_core.h"
#include "src/coreset.h"
#include "src/filesqlinfo.h"
#include "src/shotfilesqlinfo.h"

#include <QDebug>

test_core::test_core()
{

}

void test_core::init()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    set.init();

}

void test_core::test_set_query()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    qDebug() << set.getCacheRoot().absolutePath();
}

void test_core::test_set_synpath()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    for (doCore::synPath_struct &p:set.getSynDir()){
        qDebug() <<  "\n local -->" <<  p.local << "\n server-->" << p.server;

    }

}

void test_core::test_create_eps()
{
    doCore::coreSet& set = doCore::coreSet::getCoreSet();
    set.setProjectname("test_db");
    set.initdb();

    doCore::shotFileSqlInfo t;
    doCore::QfileListPtr list;
    list.append(QFileInfo("D:/tmp/etr.vdb"));
    t.setFileList(list);
    t.setInfoP(QString::fromStdString("test"));
    t.setVersionP(0);
    try {
//        t.insert();
    }  catch (std::runtime_error err) {
        qDebug() << err.what();
    }

}
