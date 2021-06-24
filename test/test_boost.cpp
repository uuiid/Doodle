//
// Created by teXiao on 2020/11/10.
//
#include <DoodleLib/DoodleLib.h>
#include <gtest/gtest.h>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <date/date.h>
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

TEST(dboost, normalize_path) {
  boost::filesystem::path root(R"(c:\some\deep\application\folder)");
  boost::filesystem::path subdir("..\\configuration\\instance");
  boost::filesystem::path cfgfile("..\\instance\\myfile.cfg");

  boost::filesystem::path tmp(root / subdir / cfgfile);

  std::cout << "normalize : " << tmp.normalize().generic_string() << "\n"
            << "lexically_normal :" << tmp.lexically_normal().generic_string() << std::endl;
}

TEST(dboost, filesys_last_write_time) {
  using namespace doodle;
  FSys::path source_path{L"F:/测试文件.mp4"};
  try {
    std::cout << "0: " << date::format("%Y-%m-%d %X",FSys::last_write_time_point(source_path)) << std::endl;

  } catch (const FSys::filesystem_error &e) {
    std::cout << e.what()
              << " \nutf : "
              << ""
              << std::endl;
  }
  struct _stat64 fileInfo {};
  if (_wstat64(source_path.generic_wstring().c_str(), &fileInfo) != 0) {
    std::cout << "Error : not find last_write_time " << std::endl;
  }
  auto k_t  = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
  auto k_t4 = std::chrono::system_clock::from_time_t(FSys::last_write_time_t(source_path));

  date::format("%Y-%m-%d %X",k_t);
  std::cout << "\n"
            << fileInfo.st_mtime << "\n"
            << "k_t: " << date::format("%Y-%m-%d %X",k_t) << "\n"
            << "k_t == k_t4: " << ((k_t == k_t4 ) ? "ok": "not")<< "\n"
            << std::endl;
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

TEST(dboost, split) {
  std::vector<std::string> str;
  boost::split(str, "test;123;tt;22;", boost::is_any_of(";"), boost::token_compress_on);
  std::cout << "size: " << str.size() << std::endl;
  for (auto &sub : str) {
    std::cout << sub << std::endl;
  }
}
TEST(dboost, filesys_append) {
  boost::filesystem::path path{"D:/tse/tset.txrt"};
  // path.append(".backup");
  path.replace_extension(".txt.backup");
  std::cout << path << std::endl;
}

TEST(dboost, path_append) {
  doodle::FSys::path p1{L"D:\\9-houqi\\Content\\Character"};
  doodle::FSys::path p2{L"D:/"};
  std::cout << "root name: " << p1.root_name() << std::endl
            << "root path: " << p1.root_path() << std::endl
            << "root dir: " << p1.root_directory() << std::endl
            << "relative path: " << p1.relative_path() << std::endl
            << "p1.root_name() / p1.root_directory() / p1.relative_path(): "
            << p1.root_name() / p1.root_directory() / p1.relative_path() << std::endl
            << R"( D:\9-houqi\Content\Character (lexically_relative) D:/ : )"
            << p1.lexically_relative(p2) << std::endl
            << R"(D:/ (lexically_relative) D:\9-houqi\Content\Character  : )"
            << p2.lexically_relative(p1) << std::endl
            << R"(D:\9-houqi\Content\Character (lexically_proximate) D:/ : )"
            << p1.lexically_proximate(p2) << std::endl
            << R"(D:/ (lexically_proximate) D:\9-houqi\Content\Character : )"
            << p2.lexically_proximate(p1) << std::endl;
  ;
}
