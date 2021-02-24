﻿#include <gtest/gtest.h>
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
  auto prj = std::make_shared<doodle::Project>("V:/");
  set.setProject(prj);
}

void CoreTest::TearDown() {
}

TEST_F(CoreTest, getInfo) {
  std::cout
      << "\nuser : " << set.getUser()
      << "\nuser_en : " << set.getUser_en()
      << "\ndoc : " << set.getDoc()
      << "\ncacheRoot : " << set.getCacheRoot()
      << "\ndoc : " << set.getDepartment()
      << "\nsynpath : " << set.getSynPathLocale()
      << std::endl;
}

TEST_F(CoreTest, getProjectInfo) {
  auto prj         = set.getProject();
  auto path_parser = prj->findParser(rttr::type::get<doodle::assdepartment>());
  std::cout
      << "\n name : " << prj->Name()
      << "\n root : " << prj->Root()
      << "\n doc : " << path_parser[0]
      << std::endl;
}

TEST_F(CoreTest, setInfo) {
}

TEST_F(CoreTest, rttr_get_all_install) {
  auto types = rttr::type::get_types();
  for (auto&& it : types) {
    std::cout << "name : " << it.get_name() << std::endl;
  }
}

TEST_F(CoreTest, find_dep_type) {
}

TEST_F(CoreTest, create_shotinfo) {
}

TEST_F(CoreTest, get_shotinf) {
}

TEST_F(CoreTest, create_assInfo) {
}

TEST_F(CoreTest, get_assInf) {
}

TEST_F(CoreTest, up_maya_file) {
}
TEST_F(CoreTest, create_Move) {
}

TEST_F(CoreTest, convert_Move) {
}
TEST_F(CoreTest, Synfile_up_ue) {
}
TEST_F(CoreTest, Synfile_dow_ue) {
}
TEST_F(CoreTest, Synfile) {
}
TEST_F(CoreTest, Synfile_lisgt) {
}
TEST_F(CoreTest, Synfile_create_dir) {
}
