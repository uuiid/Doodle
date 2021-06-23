#include <DoodleLib/DoodleLib.h>
#include <gtest/gtest.h>

TEST(pinyinlib, convert) {
  auto trs = doodle::convert::Get().toEn("aa大.?小d多dd53少");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("林奇");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("李叶华");
  std::cout << trs << std::endl;
}

TEST(pinyinlib, fmt) {
  std::cout << fmt::format(L"{}", std::wstring{L"test"}) << "\n"
            << fmt::format("{:04d}", 2) << "\n"
            << fmt::format("{}", doodle::FSys::path{"test"}) << "\n"
            << fmt::format(L"{}", doodle::FSys::path{L"还会"}) << "\n"
            << std::endl;
}
TEST(core, create_path) {
  using namespace doodle;
  auto k_1 = std::make_shared<Project>("D:/", "ttt");
  MetadataSet::Get().installProject(k_1);
  MetadataSet::Get().setProject_(k_1);
  auto k_2 = std::make_shared<Assets>(k_1, "ttt");
  k_1->addChildItem(k_2);
  auto k_3 = std::make_shared<Assets>(k_2, "eee");
  k_2->addChildItem(k_3);
  auto path = AssetsPath("D:/err/tmp.exe", k_3);
  std::cout << " LocalPath: " << path.getLocalPath() << "\n"
            << " ServerPath: " << path.getServerPath() << "\n"
            << std::endl;
}
