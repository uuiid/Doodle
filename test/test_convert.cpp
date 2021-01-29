#include <pinyinlib/convert.h>
#include <gtest/gtest.h>

TEST(pinyinlib, convert) {
  auto trs = dopinyin::convert::Get().toEn("aa大.?小d多dd53少");
  std::cout << trs << std::endl;
}
