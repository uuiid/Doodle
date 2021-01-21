//
// Created by teXiao on 2020/11/10.
//

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <gtest/gtest.h>
#include <boost/regex.hpp>

std::string encode64(const std::string &val) {
  using namespace boost::archive::iterators;
  using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
  auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
  return tmp.append((3 - val.size() % 3) % 3, '=');
}
std::string decode64(const std::string &val) {
  using namespace boost::archive::iterators;
  using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
  return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
    return c == '\0';
  });
}

TEST(dboost ,test_encode){
  std::cout << encode64("zhangyubin") <<std::endl;
  ASSERT_TRUE(encode64("zhangyubin")=="emhhbmd5dWJpbg==");
}

TEST(dboost, test_decode){
  ASSERT_TRUE(decode64("MTIzNDU=")=="12345");
}

TEST(dboost,test_rex){
  auto rex = boost::regex(R"(Anm|Animation)");
  ASSERT_TRUE(boost::regex_match("Anm",rex));
}

