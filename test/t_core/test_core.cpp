#include "test_core.h"
#include "src/coreset.h"
#include "src/filesqlinfo.h"

#include "src/episodes.h"
#include "src/shot.h"
#include "src/fileclass.h"
#include "src/filetype.h"
#include "src/assType.h"

#include "src/assFileSqlInfo.h"
#include "src/shotfilesqlinfo.h"

#include <QDebug>

test_core::test_core()
{
}

void test_core::init()
{
    doCore::coreSet &set = doCore::coreSet::getCoreSet();
    set.init();
    set.setProjectname("test_db");
    set.initdb();
}

void test_core::test_set_query()
{
    doCore::coreSet &set = doCore::coreSet::getCoreSet();
    qDebug() << set.getCacheRoot().absolutePath();
}

void test_core::test_set_synpath()
{
    doCore::coreSet &set = doCore::coreSet::getCoreSet();
    for (doCore::synPath_struct &p : set.getSynDir())
    {
        qDebug() << "\n local -->" << p.local << "\n server-->" << p.server;
    }
}

void test_core::test_create_shotinfo()
{
    doCore::episodesPtrList eplist;
    eplist = doCore::episodes::getAll();
    if (eplist.empty())
    {
        doCore::episodesPtr eps(new doCore::episodes());
        eps->setEpisdes(10);
        eps->insert();

        doCore::shotPtr sh(new doCore::shot());
        sh->setShot(10);
        sh->setEpisdes(eps.toWeakRef());
        sh->insert();

        doCore::fileClassPtr fc(new doCore::fileClass());
        fc->setFileclass(doCore::fileClass::e_fileclass::VFX);
        fc->setEpisodes(eps.toWeakRef());
        fc->setShot(sh.toWeakRef());
        fc->insert();

        doCore::fileTypePtr ft(new doCore::fileType());
        ft->setType("test");
        ft->setFileClass(fc.toWeakRef());
        ft->setShot(sh.toWeakRef());
        ft->insert();

        doCore::shotFileSqlInfoPtr sf(new doCore::shotFileSqlInfo());
        doCore::QfileListPtr list;
        sf->setInfoP(QString("test"));
        list.append(QFileInfo("D:/tmp/etr.vdb"));
        sf->setFileList(list);
        sf->setVersionP(0);
        sf->setEpisdes(eps.toWeakRef());
        sf->setShot(sh.toWeakRef());
        sf->setFileclass(fc.toWeakRef());
        sf->setFileType(ft.toWeakRef());
        sf->insert();
    }
    else
    {
        qDebug() << "is create ok, ";
    }
}

void test_core::test_get_shotinfo()
{
    doCore::episodesPtrList eplist;
    eplist = doCore::episodes::getAll();
    if (!eplist.isEmpty())
    {
        qDebug() << "episodes: " << eplist[0]->getEpisdes();
    }
}

void test_core::test_create_assInfo()
{
}

void test_core::test_get_assInfo()
{
    doCore::fileClassPtrList list_fileClass;
    list_fileClass = doCore::fileClass::getAll();
}