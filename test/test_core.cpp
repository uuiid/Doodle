#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "Logger.h"

#include <iostream>
#include <memory>

#include <rttr/type>
#include <core_doQt.h>
class CoreTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  doCore::coreSet& set = doCore::coreSet::getSet();
};

void CoreTest::SetUp() {
  set.init();
  set.setProjectname((std::string) "dubuxiaoyao3");
  set.initdb();
}

void CoreTest::TearDown() {}

TEST_F(CoreTest, setInfo) {
  std::cout
      << "sql ip : " << set.getIpMysql() << std::endl
      << "ftp ip : " << set.getIpFtp() << std::endl
      << "program_location : " << set.program_location() << std::endl
      << "get syn path local : " << set.getSynPathLocale() << std::endl
      << "user : " << set.getUser() << std::endl
      << "user en : " << set.getUser_en() << std::endl
      << "Department : " << set.getDepartment() << std::endl
      << "syn freeSynfile : " << set.getFreeFileSyn() << std::endl
      << "syn eps : " << set.getSyneps() << std::endl
      << "doc root : " << set.getDoc() << std::endl
      << "project root : " << set.getPrjectRoot() << std::endl
      << "project : " << set.getProjectname() << std::endl
      << "shot root : " << set.getShotRoot() << std::endl
      << "ass root : " << set.getAssRoot() << std::endl
      << "cache root : " << set.getCacheRoot() << std::endl;
}

TEST_F(CoreTest, rttr_get_all_install) {
  doCore::shotPtrList shotList;
  for (unsigned int i = 0; i < 10; i++) {
    auto s = std::make_shared<doCore::shot>();
    s->setShot(i);
    shotList.push_back(s);
  }
  auto allShotClass = rttr::type::get_types();
  for (auto&& i : allShotClass) {
    std::cout << "name " << i.get_name() << std::endl;
  }
}

TEST_F(CoreTest, find_dep_type) {
}

TEST_F(CoreTest, create_shotinfo) {
  doCore::episodesPtr eps(new doCore::episodes());
  eps->setEpisdes(250);
  eps->insert();
  std::cout << "eps " << eps->getEpisdes() << std::endl;
  std::cout << "eps id " << eps->getIdP() << std::endl;

  doCore::shotPtr sh(new doCore::shot());
  sh->setShot(10);
  sh->setEpisodes(eps);
  sh->insert();
  std::cout << "sh " << sh->getShot() << std::endl;
  std::cout << "sh id " << sh->getIdP() << std::endl;

  doCore::shotInfoPtr sf(new doCore::shotFileSqlInfo());
  doCore::dpathList list;
  sf->setInfoP("test");
  list.push_back("D:/tmp/etr.vdb");
  sf->setFileList(list);
  sf->setVersionP(0);
  sf->insert();

  std::cout << "info " << sf->getInfoP()[0] << std::endl;
  std::cout << "path " << sf->getFileList()[0] << std::endl;

  sf->deleteSQL();
  sh->deleteSQL();
  eps->deleteSQL();
}

TEST_F(CoreTest, get_shotinf) {
  doCore::episodesPtrList eplist;
  eplist = doCore::episodes::getAll();
  if (!eplist.empty()) {
    doCore::episodesPtr ep = eplist[0];
    auto shlist = doCore::shot::getAll(eplist.front());
    ASSERT_TRUE(!shlist.empty());
    auto shclList = doCore::shotClass::getAll();
    ASSERT_TRUE(!shclList.empty());
    auto shtyList = doCore::shotType::getAll();
    ASSERT_TRUE(!shtyList.empty());
    auto sfList = doCore::shotFileSqlInfo::getAll(shlist.front());
    ASSERT_TRUE(!sfList.empty());

    std::cout << "episodes: " << ep->getEpisdes_str() << std::endl;
    std::cout << "shot:" << shlist[0]->getShotAndAb_str() << std::endl;
    std::cout << "fileclass :" << shclList.front()->getClass_str() << std::endl;
    std::cout << "filetype :" << shtyList.front()->getType() << std::endl;
    std::cout << "shotinfo generatePath :" << sfList.front()->generatePath("test", ".mb") << std::endl;
    for (auto& x : sfList.front()->getFileList()) {
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
    ft_->setType((std::string) "ffff");
    ft_->insert();

    doCore::assInfoPtr sf_(new doCore::assFileSqlInfo);
    doCore::dpathList list;
    sf_->setInfoP("test");
    list.push_back("D:/tmp/etr.vdb");
    sf_->setFileList(list);
    sf_->setVersionP(1);

    sf_->insert();

    sf_->deleteSQL();
    ft_->deleteSQL();
    af_->deleteSQL();
  }
}

TEST_F(CoreTest, get_assInf) {
  auto list_fileClass = doCore::assdepartment::getAll();
  for (auto&& x : list_fileClass) {
    std::cout << "fileclass :" << x->getAssDep() << std::endl;
  }
  auto test = doCore::assClass::getAll(list_fileClass[0]);
  doCore::assClassPtr af_ = test[0];
  std::cout << "asstype :" << af_->getAssClass(true) << std::endl;
  RecordProperty("asstype", af_->getAssClass(true));
  // QTextCodec *code = QTextCodec::codecForName("GBK");
  // std::cout << "asstype :" <<code->fromUnicode(af_->getAssClass(af_)) << std::endl;

  doCore::assTypePtr ft_ = doCore::assType::getAll()[0];
  std::cout << "filetype :" << ft_->getType() << std::endl;

  doCore::assInfoPtr ai_ = doCore::assFileSqlInfo::getAll(af_)[0];
  std::cout << "assinfo path :" << ai_->generatePath("test", ".mb") << std::endl;
}

TEST_F(CoreTest, up_maya_file) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto maya = std::make_shared<doCore::mayaArchive>(shotinfo);
  maya->update("D:/DBXY_004_035.mb");

  shotinfo->deleteSQL();
}
TEST_F(CoreTest, mayaExport_fbx) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  for (const auto& item : shtypeList) {
    if (item->getType() == "fbx") {
      auto shot = doCore::shotFileSqlInfo::getAll(shotList.front());
      auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
      shotinfo->setShotType(item);
      auto export_ = std::make_shared<doCore::mayaArchiveShotFbx>(shotinfo);
      ASSERT_TRUE(export_->update(shot.front()->getFileList().front()));
    }
  }
}
TEST_F(CoreTest, create_Move) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doCore::moveShotA>(shotinfo);
  up_move->update({"D:\\sc_064"});

  shotinfo->deleteSQL();
}

TEST_F(CoreTest, convert_Move) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doCore::moveShotA>(shotinfo);
  up_move->update({"D:\\DBXY_041_017_AN.mov"});

  shotinfo->deleteSQL();
}
TEST_F(CoreTest, Synfile_up_ue) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  auto shotinfo = std::make_shared<doCore::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());

  auto up_move = std::make_shared<doCore::ueArchive>(shotinfo);
  up_move->update(R"(F:\Users\teXiao\Documents\Unreal Projects\test_tt\test_tt.uproject)");
}
TEST_F(CoreTest, Synfile_dow_ue) {
  auto epslist = doCore::episodes::getAll();
  auto shotList = doCore::shot::getAll(epslist.front());
  auto shclassList = doCore::shotClass::getAll();
  auto shtypeList = doCore::shotType::getAll();

  auto shotinfoList = doCore::shotFileSqlInfo::getAll(shotList.front());
  auto up_move = std::make_shared<doCore::ueArchive>(shotinfoList.front());
  up_move->down(R"(F:\Users\)");
  shotinfoList.front()->deleteSQL();
}
TEST_F(CoreTest, Synfile) {
  set.setSyneps(41);
  doCore::ueSynArchive().syn(nullptr);
}
TEST_F(CoreTest, Synfile_lisgt) {
  set.setSyneps(41);
  set.setDepartment((std::string) "Light");
  doCore::ueSynArchive().syn(nullptr);
}
TEST_F(CoreTest, Synfile_create_dir) {
  set.setSyneps(41);
  set.setAssRoot("/tmp/tt");
  // doCore::ueSynArchive().makeDir(<#initializer#>);
}
