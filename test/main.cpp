#include "core_global.h"
#include "src/coreset.h"
#include "Logger.h"

#include <QCoreApplication>
#include <QTextCodec>
#include <QSqlQuery>
#include <gtest/gtest.h>

#include <iostream>
class Environment : public ::testing::Environment {
 public:

  void SetUp() override;
  void TearDown() override;
  doCore::coreSet &set = doCore::coreSet::getCoreSet();
};

void Environment::SetUp() {
  set.init();
  set.setProjectname("test_db");
  set.initdb();
  std::cout << "exe init_" << std::endl;
  doCore::sqlQuertPtr query = doCore::coreSql::getCoreSql().getquery();
  if (!query->exec("INSERT INTO test_db.configure(name, value) VALUES "
                   "('shotRoot','/03_Workflow/Shots'),"
                   "('assetsRoot','/03_Workflow/Assets'),"
                   "('synSever','/03_Workflow/Assets'),"
                   "('project','X:/');"))
    std::cout << "not exe insert project info" << std::endl;
  if (!query->exec("INSERT INTO test_db.configure(name, value, value2, value3, value4) VALUES"
                   "('synpath','Light','001','Left','Ep_01/FuTuJieShenYuan_ZX/Content/shot'),"
                   "('synpath','Light','001','Right','Ep_01/FuTuJieShenYuan_ZX/Content/shot'),"
                   "('synpath','VFX','001','Left','Ep_01/HuaiLaoBanDeDain_LZ/Content/shot'),"
                   "('synpath','VFX','001','Right','Ep_01/HuaiLaoBanDeDain_LZ/Content/shott');"))
    std::cout << "not exe insert syninfo" << std::endl;
  if (!query->exec("INSERT INTO test_db.fileclass(file_class) VALUES "
                   "('character'),('effects'),('scene'),('prop')"))
    std::cout << "not exe insert fileclass info" << std::endl;
}

void Environment::TearDown() {
//    set.setProjectname("test_db");
//    set.initdb();
//    doCore::sqlQuertPtr query = doCore::coreSql::getCoreSql().getquery();
//    if (!query->exec("DELETE test_db.basefile FROM test_db.basefile;"))
//        std::cout << "not exe delete SQL basefile" << std::endl;
//    if (!query->exec("DELETE test_db.znch FROM test_db.znch;"))
//        std::cout << "not exe delete SQL znch" << std::endl;
//    if (!query->exec("DELETE test_db.filetype FROM test_db.filetype;"))
//        std::cout << "not exe delete SQL filetype" << std::endl;
//    if (!query->exec("DELETE test_db.assclass FROM test_db.assclass;"))
//        std::cout << "not exe delete SQL assclass" << std::endl;
//    if (!query->exec("DELETE test_db.fileclass FROM test_db.fileclass;"))
//        std::cout << "not exe delete SQL fileclass" << std::endl;
//    if (!query->exec("DELETE test_db.shot FROM test_db.shot;"))
//        std::cout << "not exe delete SQL shot" << std::endl;
//    if (!query->exec("DELETE test_db.episodes FROM test_db.episodes;"))
//        std::cout << "not exe delete SQL episodes" << std::endl;
//    if (!query->exec("DELETE test_db.configure FROM test_db.configure;"))
//        std::cout << "not exe delete SQL configure" << std::endl;
}

int main(int argc, char *argv[]) {
  //创建qt必要的运行事件循环
  QCoreApplication app(argc, argv);
  QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
  //初始化log
  Logger::doodle_initLog();
  //设置本地编码
  QTextCodec *codec = QTextCodec::codecForName("GBK");
  QTextCodec::setCodecForLocale(codec);

  //初始化测试环境
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new Environment);
  return RUN_ALL_TESTS();
}
