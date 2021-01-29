#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <loggerlib/Logger.h>

#include <iostream>
#include <memory>

#include <rttr/type>
#include <corelib/core_Cpp.h>
class CoreTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  doodle::coreSet& set = doodle::coreSet::getSet();
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
  doodle::shotPtrList shotList;
  for (unsigned int i = 0; i < 10; i++) {
    auto s = std::make_shared<doodle::shot>();
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
  doodle::episodesPtr eps(new doodle::episodes());
  eps->setEpisdes(250);
  eps->insert();
  std::cout << "eps " << eps->getEpisdes() << std::endl;
  std::cout << "eps id " << eps->getIdP() << std::endl;

  doodle::shotPtr sh(new doodle::shot());
  sh->setShot(10);
  sh->setEpisodes(eps);
  sh->insert();
  std::cout << "sh " << sh->getShot() << std::endl;
  std::cout << "sh id " << sh->getIdP() << std::endl;

  doodle::shotInfoPtr sf(new doodle::shotFileSqlInfo());
  doodle::dpathList list;
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
  doodle::episodesPtrList eplist;
  eplist = doodle::episodes::getAll();
  ASSERT_TRUE(!eplist.empty());
  std::cout << "episodes: " << eplist.front()->getEpisdes_str() << std::endl;
  for (auto& i : doodle::episodes::Instances()) {
    std::cout << " eps:" << i->getEpisdes_str();
  }
  std::cout << std::endl;

  auto shlist = doodle::shot::getAll(eplist.front());
  ASSERT_TRUE(!shlist.empty());
  std::cout << "shot:" << shlist[0]->getShotAndAb_str() << std::endl;
  for (auto&& i : doodle::shot::Instances()) {
    std::cout << " shot:" << i->getShotAndAb_str();
  }
  std::cout << std::endl;

  auto shclList = doodle::shotClass::getAll();
  ASSERT_TRUE(!shclList.empty());
  std::cout << "fileclass :" << shclList.front()->getClass_str() << std::endl;
  for (auto&& i : doodle::shotClass::Instances()) {
    std::cout << " fileclass:" << i->getClass_str();
  }
  std::cout << std::endl;

  auto shtyList = doodle::shotType::getAll();
  ASSERT_TRUE(!shtyList.empty());
  std::cout << "filetype :" << shtyList.front()->getType() << std::endl;
  for (auto&& i : doodle::shotType::Instances()) {
    std::cout << " shottype:" << i->getType() << "\n\r"
              << std::endl;
  }

  auto sfList = doodle::shotFileSqlInfo::getAll(shlist.front());
  ASSERT_TRUE(!sfList.empty());
  std::cout << "shotinfo generatePath :" << sfList.front()->generatePath("test", ".mb") << std::endl;
  for (auto& i : doodle::shotFileSqlInfo::Instances()) {
    std::cout << " shotinfo:" << i->getFileList().front() << "\n"
              << std::endl;
  }

  for (auto& x : sfList.front()->getFileList()) {
    std::cout << "shotinfo path :" << x.generic_string() << std::endl;
  }
}

TEST_F(CoreTest, create_assInfo) {
  auto fc_ = doodle::assdepartment::getAll();
  ASSERT_TRUE(fc_.size() == 4);
  doodle::assClassPtr af_(new doodle::assClass);
  af_->setAssDep(fc_[0]);
  af_->setAssClass("大小", true);
  af_->insert();

  doodle::assTypePtr ft_(new doodle::assType);
  ft_->setType((std::string) "ffff");
  ft_->insert();

  doodle::assInfoPtr sf_(new doodle::assFileSqlInfo);
  doodle::dpathList list;
  sf_->setInfoP("test");
  list.push_back("D:/tmp/etr.vdb");
  sf_->setFileList(list);
  sf_->setVersionP(1);

  sf_->insert();

  sf_->deleteSQL();
  ft_->deleteSQL();
  af_->deleteSQL();
}

TEST_F(CoreTest, get_assInf) {
  auto list_fileClass = doodle::assdepartment::getAll();
  for (auto&& x : list_fileClass) {
    std::cout << "fileclass :" << x->getAssDep() << std::endl;
  }
  auto test               = doodle::assClass::getAll(list_fileClass[0]);
  doodle::assClassPtr af_ = test[0];
  std::cout << "asstype :" << af_->getAssClass(true) << std::endl;
  RecordProperty("asstype", af_->getAssClass(true));
  // QTextCodec *code = QTextCodec::codecForName("GBK");
  // std::cout << "asstype :" <<code->fromUnicode(af_->getAssClass(af_)) << std::endl;

  doodle::assTypePtr ft_ = doodle::assType::getAll()[0];
  std::cout << "filetype :" << ft_->getTypeS() << std::endl;

  doodle::assInfoPtr ai_ = doodle::assFileSqlInfo::getAll(af_)[0];
  std::cout << "assinfo path :" << ai_->generatePath("test", ".mb") << std::endl;
}

TEST_F(CoreTest, up_maya_file) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  auto shotinfo = std::make_shared<doodle::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto maya = std::make_shared<doodle::mayaArchive>(shotinfo);
  maya->update("D:/DBXY_004_035.mb");

  shotinfo->deleteSQL();
}
TEST_F(CoreTest, mayaExport_fbx) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  for (const auto& item : shtypeList) {
    if (item->getType() == "fbx") {
      auto shot     = doodle::shotFileSqlInfo::getAll(shotList.front());
      auto shotinfo = std::make_shared<doodle::shotFileSqlInfo>();
      shotinfo->setShotType(item);
      auto export_ = std::make_shared<doodle::mayaArchiveShotFbx>(shotinfo);
      ASSERT_TRUE(export_->update(shot.front()->getFileList().front()));
    }
  }
}
TEST_F(CoreTest, create_Move) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  auto shotinfo = std::make_shared<doodle::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doodle::moveShotA>(shotinfo);
  up_move->update({"D:\\sc_064"});

  shotinfo->deleteSQL();
}

TEST_F(CoreTest, convert_Move) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  auto shotinfo = std::make_shared<doodle::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());
  auto up_move = std::make_shared<doodle::moveShotA>(shotinfo);
  up_move->update({"D:\\DBXY_041_017_AN.mov"});

  shotinfo->deleteSQL();
}
TEST_F(CoreTest, Synfile_up_ue) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  auto shotinfo = std::make_shared<doodle::shotFileSqlInfo>();
  shotinfo->setShotType(shtypeList.front());

  auto up_move = std::make_shared<doodle::ueArchive>(shotinfo);
  up_move->update(R"(F:\Users\teXiao\Documents\Unreal Projects\test_tt\test_tt.uproject)");
}
TEST_F(CoreTest, Synfile_dow_ue) {
  auto epslist     = doodle::episodes::getAll();
  auto shotList    = doodle::shot::getAll(epslist.front());
  auto shclassList = doodle::shotClass::getAll();
  auto shtypeList  = doodle::shotType::getAll();

  auto shotinfoList = doodle::shotFileSqlInfo::getAll(shotList.front());
  auto up_move      = std::make_shared<doodle::ueArchive>(shotinfoList.front());
  up_move->down(R"(F:\Users\)");
  shotinfoList.front()->deleteSQL();
}
TEST_F(CoreTest, Synfile) {
  set.setSyneps(41);
  doodle::ueSynArchive().syn(nullptr, nullptr);
}
TEST_F(CoreTest, Synfile_lisgt) {
  set.setSyneps(41);
  set.setDepartment((std::string) "Light");
  doodle::ueSynArchive().syn(nullptr, nullptr);
}
TEST_F(CoreTest, Synfile_create_dir) {
  set.setSyneps(41);
  set.setAssRoot("/tmp/tt");
  //  ueSynArchive().makeDir(<#initializer#>);
}
