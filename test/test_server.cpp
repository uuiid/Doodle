﻿//
// Created by TD on 2021/5/26.
//

#include <DoodleLib/DoodleLib.h>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

TEST(Server, createPrj) {
  doodle::CoreSet::getSet().guiInit();

  auto k_f = std::make_shared<doodle::MetadataFactory>();
  auto prj = std::make_shared<doodle::Project>("D:/", "测试");
  prj->insert_into(k_f);
  std::cout << prj->getId() << std::endl;
  prj = std::make_shared<doodle::Project>("D:/", "测试2");
  prj->insert_into(k_f);

  auto& k_m = doodle::MetadataSet::Get();
  doodle::CoreSet::getSet().guiInit();

  for (const auto& prj : k_m.getAllProjects()) {
    std::cout << "id: " << prj->getId() << "\n"
              << "uuid: " << prj->getUUID() << "\n"
              << "name: " << prj->getName() << "\n"
              << "path: " << prj->getPath() << "\n";
  }
  std::cout << std::endl;

  ASSERT_TRUE(k_m.getAllProjects().size() == 2);
}
#include <date/date.h>

TEST(Server, dow_updata) {
  using namespace doodle;
  doodle::CoreSet::getSet().guiInit();

  auto k_ch = doodle::CoreSet::getSet().getRpcFileSystemClient();
  k_ch->Upload("D:/WinDev2012Eval.VirtualBox.zip", "test/test.file.zip");
  auto [k_t_ex, k_t_dir] = k_ch->IsFolder("test");
  auto [k_f_ex, k_f_dir] = k_ch->IsFolder("test/test.file.zip");
  std::cout << "is ex: " << k_ch->IsExist("test/test.file.zip")
            << "test is ex: " << k_t_ex
            << "test is folder: " << k_t_dir
            << "test/test.file.zip is ex: " << k_t_ex
            << "test/test.file.zip is folder: " << k_t_dir
            << "test/test.file.zip time: " << date::format("%Y/%m/%d %H:%M", k_ch->GetTimestamp("test/test.file.zip"))
            << "test/test.file.zip size: " << k_ch->GetSize("test/test.file.zip")
            << std::endl;

  k_ch->Download("D:/WinDev2012Eval_test.VirtualBox.zip", "test/test.file.zip");
}
