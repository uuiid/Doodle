//
// Created by TD on 2021/5/26.
//

#include <DoodleLib/DoodleLib.h>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

TEST(Server, createPrj) {
  DoodleLib::Get().init_gui();

  auto k_f = std::make_shared<doodle::MetadataFactory>();
  auto prj = std::make_shared<doodle::Project>("D:/", "测试");
  prj->insert_into(k_f);
  std::cout << prj->getId() << std::endl;
  prj = std::make_shared<doodle::Project>("D:/", "测试2");
  prj->insert_into(k_f);

  DoodleLib::Get().init_gui();

  for (const auto& k_prj : doodle::DoodleLib::Get().p_project_vector) {
    std::cout << "id: " << k_prj->getId() << "\n"
              << "uuid: " << k_prj->getUUID() << "\n"
              << "name: " << k_prj->getName() << "\n"
              << "path: " << k_prj->getPath() << "\n";
  }
  std::cout << std::endl;

  ASSERT_TRUE(doodle::DoodleLib::Get().p_project_vector.size() == 2);
}
#include <date/date.h>

#include <memory>

TEST(Server, dow_updata) {
  using namespace doodle;
  DoodleLib::Get().init_gui();

  auto k_ch = doodle::DoodleLib::Get().getRpcFileSystemClient();

  rpc_trans_path_ptr_list k_list;
  k_list.emplace_back(std::make_unique<rpc_trans_path>("D:\\test3.mp4", "test/test.file.7z", "test_backup_path2/tset.mp4"));
  k_list.emplace_back(std::make_unique<rpc_trans_path>("D:\\Kitchen_set", "test/tmp", "test_backup_path2/"));
  k_list.emplace_back(std::make_unique<rpc_trans_path>("D:\\Kitchen_set", "test/tmp1", "test_backup_path2/"));
  k_list.emplace_back(std::make_unique<rpc_trans_path>("D:\\Kitchen_set", "test/tmp2", "test_backup_path2/"));
  k_list.emplace_back(std::make_unique<rpc_trans_path>("D:\\Kitchen_set", "test/tmp", "test_backup_path2/"));

  auto k_item = k_ch->Upload(k_list);
  k_item->get_term()->sig_progress.connect([k_item](std::double_t in_) {
    DOODLE_LOG_INFO(" 进度 {}", k_item->get_term()->step(in_));
  });
  k_item->get_term()->sig_message_result.connect([](const std::string& in_) {
    DOODLE_LOG_INFO(" 消息 {}", in_);
  });
  k_item->get_term()->sig_finished.connect([]() {
    DOODLE_LOG_INFO(" -->完成上传");
  });

  (*k_item)();
  k_item->wait();
  auto [k_t_ex, k_t_dir] = k_ch->IsFolder("test");
  auto [k_f_ex, k_f_dir] = k_ch->IsFolder("test/test.file.7z");
  std::cout << "is ex: " << k_ch->IsExist("test/test.file.7z") << "\n"
            << "test is ex: " << k_t_ex << "\n"
            << "test is folder: " << k_t_dir << "\n"
            << "test/test.file.7z is ex: " << k_t_ex << "\n"
            << "test/test.file.7z is folder: " << k_f_dir << "\n"
            << "test/test.file.7z time: " << date::format("%Y/%m/%d %H:%M", k_ch->GetTimestamp("test/test.file.7z")) << "\n"
            << "test/test.file.7z size: " << k_ch->GetSize("test/test.file.7z") << "\n"
            << std::endl;
  doodle::CoreSet::getSet().clear();
  k_list.clear();
  k_list.emplace_back(rpc_trans_path_ptr{new rpc_trans_path{"D:\\test3.mp4", "test/test.file.7z", "test_backup_path2/tset.mp4"}});
  k_list.emplace_back(rpc_trans_path_ptr{new rpc_trans_path{"D:\\Kitchen_set2", "test/tmp", "test_backup_path2/"}});

  k_item = k_ch->Download(k_list);
  k_item->get_term()->sig_progress.connect([k_item](std::double_t in_) {
    DOODLE_LOG_INFO(" 进度 {}", k_item->get_term()->step(in_));
  });
  k_item->get_term()->sig_message_result.connect([](const std::string& in_) {
    DOODLE_LOG_INFO(" 消息 {}", in_);
  });
  k_item->get_term()->sig_finished.connect([]() {
    DOODLE_LOG_INFO(" -->完成下载");
  });
  (*k_item)();

  k_item->wait();
}
