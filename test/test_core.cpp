#include "src/coreset.h"
#include "src/filesqlinfo.h"

#include "src/episodes.h"
#include "src/shot.h"
#include "src/shotClass.h"
#include "src/shottype.h"

#include "src/assClass.h"
#include "src/assType.h"
#include "src/assdepartment.h"
#include "src/assfilesqlinfo.h"
#include "src/shotfilesqlinfo.h"
#include "src/moveShotA.h"
#include "src/mayaArchive.h"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "Logger.h"

#include <iostream>
#include <src/movieArchive.h>
#include <src/ueArchive.h>



class CoreTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  doCore::coreSet &set = doCore::coreSet::getSet();
};

void CoreTest::SetUp() {
  set.init();
  set.setProjectname("dubuxiaoyao3");
  set.initdb();
}

void CoreTest::TearDown() {

}

TEST_F(CoreTest, tets_quert) {
  std::cout << set.getCacheRoot().generic_string() << std::endl;
  RecordProperty("cacheRoot", set.getCacheRoot().generic_string());
}

TEST_F(CoreTest, set_synpath) {
  for (doCore::synPath_struct &p : set.getSynDir()) {
    std::cout << "\n local -->" << p.local.generic_string()
              << "\n server-->" << p.server.generic_string() << std::endl;;
  }
}

TEST_F(CoreTest, create_shotinfo) {
  doCore::episodesPtrList eplist;
  eplist = doCore::episodes::getAll();
  if (!eplist.empty()) {
    doCore::episodesPtr eps(new doCore::episodes());
    eps->setEpisdes(11);
    eps->insert();

    doCore::shotPtr sh(new doCore::shot());
    sh->setShot(10);
    sh->setEpisodes(eps);
    sh->insert();

    doCore::shotClassPtr fc(new doCore::shotClass());
    fc->setclass(doCore::shotClass::e_fileclass::VFX);

    fc->setShot(sh);
    fc->insert();

    doCore::shotTypePtr ft(new doCore::shotType());
    ft->setType("test");
    ft->setFileClass(fc);

    ft->insert();

    doCore::shotInfoPtr sf(new doCore::shotFileSqlInfo());
    doCore::dpathList list;
    sf->setInfoP("test");
    list.push_back("D:/tmp/etr.vdb");
    sf->setFileList(list);
    sf->setVersionP(0);

    sf->setShotType(ft);
    sf->insert();

    sf->deleteSQL();
    ft->deleteSQL();
    fc->deleteSQL();
    sh->deleteSQL();
    eps->deleteSQL();

  }
}

TEST_F(CoreTest, get_shotinf) {
  doCore::episodesPtrList eplist;
  eplist = doCore::episodes::getAll();
  if (!eplist.empty()) {
    doCore::episodesPtr ep = eplist[0];
    auto shlist = doCore::shot::getAll(eplist.front());
    ASSERT_TRUE(!shlist.empty());
    auto shclList = doCore::shotClass::getAll(shlist.front());
    ASSERT_TRUE(!shclList.empty());
    auto shtyList = doCore::shotType::getAll(shclList.front());
    ASSERT_TRUE(!shtyList.empty());
    auto sfList = doCore::shotFileSqlInfo::getAll(shtyList.front());
    ASSERT_TRUE(!sfList.empty());

    std::cout << "episodes: " << ep->getEpisdes_str() << std::endl;
    std::cout << "shot:" << shlist[0]->getShotAndAb_str() << std::endl;
    std::cout << "fileclass :" << shclList.front()->getClass_str() << std::endl;
    std::cout << "filetype :" << shtyList.front()->getType() << std::endl;
    std::cout << "shotinfo generatePath :" << sfList.front()->generatePath("test", ".mb") << std::endl;
    for (auto &x : sfList.front()->getFileList()) {
      std::cout << "shotinfo path :" << x.generic_string() << std::endl;
    }
  }
}

TEST_F(CoreTest, create_assInfo) {
  auto fc_ = doCore::assdepartment::getAll();

  if (fc_.size() == 4) {
    doCore::assClassPtr af_(new doCore::assClass);
    af_->setAssDep(fc_[0]);
    af_->setAssClass("大小", true);
    af_->insert();

    doCore::assTypePtr ft_(new doCore::assType);
    ft_->setType("ffff");
    ft_->setAssClassPtr(af_);
    ft_->insert();

    doCore::assInfoPtr sf_(new doCore::assFileSqlInfo);
    doCore::dpathList list;
    sf_->setInfoP("test");
    list.push_back("D:/tmp/etr.vdb");
    sf_->setFileList(list);
    sf_->setVersionP(1);

    sf_->setAssType(ft_);

    sf_->insert();

    sf_->deleteSQL();
    ft_->deleteSQL();
    af_->deleteSQL();
  }
}

TEST_F(CoreTest, get_assInf) {
  auto list_fileClass = doCore::assdepartment::getAll();
  for (auto &&x : list_fileClass) {
    std::cout << "fileclass :" << x->getAssDep() << std::endl;
  }
  auto test = doCore::assClass::getAll(list_fileClass[0]);
  doCore::assClassPtr af_ = test[0];
  std::cout << "asstype :" << af_->getAssClass(true) << std::endl;
  RecordProperty("asstype", af_->getAssClass(true));
  // QTextCodec *code = QTextCodec::codecForName("GBK");
  // std::cout << "asstype :" <<code->fromUnicode(af_->getAssClass(af_)) << std::endl;

  doCore::assTypePtr ft_ = doCore::assType::getAll(af_)[0];
  std::cout << "filetype :" << ft_->getType() << std::endl;

  doCore::assInfoPtr ai_ = doCore::assFileSqlInfo::getAll(ft_)[0];
  std::cout << "assinfo path :" << ai_->generatePath("test", ".mb") << std::endl;
}

TEST_F(CoreTest,up_maya_file){
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll(shotList.front());
  auto shtypeList = doCore::shotType::getAll(shclassList.front());

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto maya = std::make_shared<doCore::mayaArchive>(shotinfo);
  maya->update("D:/DBXY_004_035.mb");

  shotinfo->deleteSQL();
}

TEST_F(CoreTest, create_Move){
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll(shotList.front());
  auto shtypeList = doCore::shotType::getAll(shclassList.front());

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doCore::moveShotA>(shotinfo);
  up_move->update({"D:\\sc_064"});

  shotinfo->deleteSQL();
}

TEST_F(CoreTest, convert_Move){
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll(shotList.front());
  auto shtypeList = doCore::shotType::getAll(shclassList.front());

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doCore::moveShotA>(shotinfo);
  up_move->update({"D:\\DBXY_041_017_AN.mov"});

  shotinfo->deleteSQL();
}
TEST_F(CoreTest,Synfile_down_ue){
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll(shotList.front());
  auto shtypeList = doCore::shotType::getAll(shclassList.front());

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());

  auto up_move = std::make_shared<doCore::ueArchive>(shotinfo);
  up_move->update(R"(F:\Users\teXiao\Documents\Unreal Projects\test_tt\test_tt.uproject)");
}