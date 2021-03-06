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

#include <DoodleLib/DoodleLib.h>
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
  boost::filesystem::path root(R"(c:\some\deep\application\folder)");
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

  std::cout << boost::format{"\n\n %|=60s|"} % "create ok next load file" << std::endl;

}
TEST(dboost, split){
  std::vector<std::string> str;
  boost::split(str, "test;123;tt;22;", boost::is_any_of(";"),boost::token_compress_on);
  std::cout << "size: " << str.size() << std::endl;
  for(auto &sub: str){
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
            << " D:\\9-houqi\\Content\\Character lexically_relative D:/ : "
            << p1.lexically_relative(p2) << std::endl
            << " D:/lexically_relative D:\\9-houqi\\Content\\Character  : "
            << p2.lexically_relative(p1) << std::endl
            << "D:\\9-houqi\\Content\\Character lexically_proximate D:/ : "
            << p1.lexically_proximate(p2) << std::endl
            << "D:/ lexically_proximate D:\\9-houqi\\Content\\Character : "
            << p2.lexically_proximate(p1) << std::endl;
  ;
}
