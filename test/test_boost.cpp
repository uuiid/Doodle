//
// Created by teXiao on 2020/11/10.
//

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <boost/regex.hpp>
#include <boost/locale.hpp>
#include <boost/format.hpp>
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

TEST(dboost, test_encode) {
  std::cout << encode64("zhangyubin") << std::endl;
  ASSERT_TRUE(encode64("zhangyubin") == "emhhbmd5dWJpbg==");
}

TEST(dboost, test_decode) {
  ASSERT_TRUE(decode64("MTIzNDU=") == "12345");
}

TEST(dboost, test_rex) {
  auto rex = boost::regex(R"(Anm|Animation)");
  ASSERT_TRUE(boost::regex_match("Anm", rex));
}

TEST(dboost, normalize_path) {
  boost::filesystem::path root("c:\\some\\deep\\application\\folder");
  boost::filesystem::path subdir("..\\configuration\\instance");
  boost::filesystem::path cfgfile("..\\instance\\myfile.cfg");

  boost::filesystem::path tmp(root / subdir / cfgfile);

  std::cout << "normalize : " << tmp.normalize().generic_string() << "\n"
            << "lexically_normal :" << tmp.lexically_normal().generic_string() << std::endl;
}

TEST(dboost, filesys_last_write_time) {
  auto soure = boost::locale::conv::to_utf<wchar_t>("F:/测试文件.mp4", "UTF-8");
  boost::filesystem::path source_path{soure};
  try {
    std::cout << "0: " << boost::filesystem::last_write_time(source_path) << std::endl;
    std::cout << "1: " << boost::filesystem::last_write_time(soure) << std::endl;

    boost::filesystem::path tmp_path{"F:/测试文件.mp4"};
    std::cout << "3: " << boost::filesystem::last_write_time(tmp_path) << std::endl;
  } catch (const boost::filesystem::filesystem_error &e) {
    std::cout << e.what()
              << " \nutf : "
              << ""
              << std::endl;
    auto str = boost::locale::conv::utf_to_utf<wchar_t>(e.code().message());
    // auto wstr = std::wstring(e.code().message().c_str());
  }
  struct _stat64 fileInfo;
  if (_wstat64(source_path.generic_wstring().c_str(), &fileInfo) != 0) {
    std::cout << "Error : not find last_write_time " << std::endl;
  }

  std::cout << "\n"
            << fileInfo.st_mtime << std::endl;
}

TEST(dboost, boost_local_backend) {
  auto local_backend = boost::locale::localization_backend_manager::global();
  for (auto &&it : local_backend.get_all_backends()) {
    std::cout << it << std::endl;
  }
}

TEST(dboost, boost_form_utf) {
  std::cout << boost::locale::conv::from_utf(std::string{"哈哈"}, "GBK") << std::endl;
  std::cout << boost::locale::conv::to_utf<char>(std::string{"哈哈"}, "UTF-8") << std::endl;
}

TEST(dboost, boost_format) {
  boost::format str{"ep%04d_sc%04d%s"};
  str % 1 % 50 % "A";
  std::cout << str.str() << std::endl;
  str.clear();
  str % -1 % -50 % "A";
  std::cout << str.str() << std::endl;
}