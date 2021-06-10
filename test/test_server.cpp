//
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

TEST(Server, dow_updata) {
  using namespace doodle;
  doodle::CoreSet::getSet().guiInit();

  auto k_ch = doodle::CoreSet::getSet().getRpcFileSystemClient();
  k_ch->Upload("");
}
