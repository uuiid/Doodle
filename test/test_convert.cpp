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
