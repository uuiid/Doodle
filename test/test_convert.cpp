#include <DoodleLib/DoodleLib.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <gtest/gtest.h>

#include <iostream>
TEST(pinyinlib, convert) {
  auto trs = doodle::convert::Get().toEn("aa大.?小d多dd53少");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("林奇");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("李叶华");
  std::cout << trs << std::endl;
}

TEST(pinyinlib, fmt) {
  std::wcout << fmt::format(L"{}", std::wstring{L"test"}) << "\n"
             << fmt::format(L"{}", doodle::FSys::path{L"还会"}) << "\n"
             << std::endl;
  std::cout << fmt::format("{:04d}", 2) << "\n"
            << fmt::format("{}", doodle::FSys::path{"test"}) << "\n"
            << std::endl;
}
TEST(core, create_path) {
  using namespace doodle;
  auto k_1 = std::make_shared<Project>("D:/", "ttt");
  DoodleLib::Get().p_project_vector.push_back(k_1);
  CoreSet::getSet().set_project(k_1);
  auto k_2 = std::make_shared<Assets>(k_1, "ttt");
  k_1->child_item.push_back_sig(k_2);
  auto k_3 = std::make_shared<Assets>(k_2, "eee");
  k_2->child_item.push_back_sig(k_3);
  auto path = AssetsPath("D:/err/tmp.exe", k_3);
  std::cout << " LocalPath: " << path.getLocalPath() << "\n"
            << " ServerPath: " << path.getServerPath() << "\n"
            << std::endl;
}
TEST(core, utf8_path) {
  using namespace doodle;
  std::locale::global(std::locale::classic());
  std::cout << "FSys::make_path(\"哈哈\"): " << FSys::make_path("D:/哈哈").generic_string() << "\n"
            << "FSys::path{\"哈哈\"}: " << FSys::path{"D:/哈哈"}.generic_string() << "\n"
            << "FSys::path{L\"哈哈\"}: " << FSys::path{L"D:/哈哈"}.generic_string() << "\n"
            << std::endl;
}
