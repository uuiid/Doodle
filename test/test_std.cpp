#include <gtest/gtest.h>
#include <fstream>
#include <corelib/core_Cpp.h>
#include <Windows.h>
#include <iostream>
#include <locale>
#include <regex>
#include <string>
#include <filesystem>
#include <codecvt>

#include <QtCore/QString>
#include <boost/locale.hpp>
// #include <boost/nowide/
TEST(DSTD, map_netDir) {
  NETRESOURCE resources{};
  resources.dwType       = RESOURCETYPE_DISK;
  resources.lpLocalName  = (LPSTR) "S:";
  resources.lpProvider   = 0;
  resources.lpRemoteName = (LPSTR) R"(\\192.168.10.250\public\CangFeng)";
  DWORD r                = WNetAddConnection2(&resources, NULL, NULL,
                               CONNECT_TEMPORARY | CONNECT_INTERACTIVE | CONNECT_COMMANDLINE | CONNECT_CRED_RESET);
  if (r != NO_ERROR) {
    std::cout << r << std::endl;
  }
  ASSERT_TRUE(r == NO_ERROR);
}

TEST(DSTD, canclel_netDir) {
  DWORD r = WNetCancelConnection2("S:", CONNECT_UPDATE_PROFILE, true);
  if (r != NO_ERROR) {
    std::cout << r << std::endl;
  }
  ASSERT_TRUE(r == NO_ERROR);
}

TEST(DSTD, dir) {
  auto r = 2740700278 / 8000000;
  std::cout << r << std::endl;
}

TEST(DSTD, stdStringLocale) {
}

TEST(DSTD, std_locale) {
  auto data = boost::locale::conv::to_utf<wchar_t>("中", "UTF-8");
  std::cout << data.size() << std::endl;
  std::wcout << data.at(0) << std::endl;
  std::wcout << data.data() << std::endl;
  const std::string data2{};
  // data2
}

TEST(DSTD, regex) {
  std::regex regex{"."};
}

TEST(DSTD, u8stringAndString) {
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  // std::setlocale(LC_ALL, "zh_CN.UTF-8");
  std::filesystem::path str{L"D:/哈哈/scoo+1235"};
  boost::filesystem::path str_b{"D:/哈哈/scoo+1235"};
  // std::cout << std::string{str2.begin(), str2.end()} << std::endl;
  std::cout << "str: " << str << std::endl;
  auto qstr = QString::fromLocal8Bit(str.generic_string().c_str());
  std::cout << "QString::fromLocal8Bit --> .generic_string().c_str(): " << qstr.toStdString() << std::endl;
  auto qstr2 = QString::fromStdString(str.generic_u8string());
  std::cout << "QString::fromStdString --> .generic_u8string(): " << qstr2.toStdString() << std::endl;

  std::cout << "generic_u8string: " << str.generic_u8string() << std::endl;
  std::cout << "generic_string: " << str.generic_string() << std::endl;
  std::cout << "boost conv: " << boost::locale::conv::utf_to_utf<char>(str.generic_wstring()) << std::endl;
  std::cout << "boost path: " << str_b << std::endl;
  std::cout << "boost path: " << str_b.generic_string() << std::endl;

  std::filesystem::create_directories(str);

  std::cout << "std::locale : " << std::locale{}.name().c_str() << std::endl;
  std::cout << "boost::locale : " << std::use_facet<boost::locale::info>(boost::locale::generator().generate("")).name() << std::endl;

  std::cout << typeid(std::filesystem::path).name() << std::endl;
  std::cout << typeid(std::cout).name() << std::endl;
}

TEST(DSTD, file_last_time) {
  auto file = std::filesystem::path{u8"D:/test2.mp4"};
  auto time = std::filesystem::last_write_time(file);

  auto time2 = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - decltype(time)::clock::now() + std::chrono::system_clock::now());
  auto time3 = std::chrono::system_clock::to_time_t(time2);
  std::string str{};
  str.resize(100);
  tm k_tm{};
  localtime_s(&k_tm, &time3);
  asctime_s(str.data(), 100, &k_tm);
  std::cout << str << std::endl;
}

TEST(DSTD, regerFind_eps_shot) {
  std::regex k_exp_epis{R"(ep_?(\d+))", std::regex_constants::icase};
  std::regex k_exp_shot{R"(sc_?(\d+)([a-z])?)", std::regex_constants::icase};
  std::string str{R"(D:\Sc_064B\sc_064a\sc064\sc_064_\BuJu.1001.png)"};
  std::string str_ep{R"(D:\Ep_064B\ep_064a\ep064\ep_064_\BuJu.1001.png)"};
  std::smatch k_match{};
  while (std::regex_search(str, k_match, k_exp_shot)) {
    std::cout << "\n匹配:" << std::endl;
    for (auto i = 0; i < k_match.size(); ++i)
      std::cout << k_match[i].str() << std::endl;

    str = k_match.suffix();
  }
  while (std::regex_search(str_ep, k_match, k_exp_epis)) {
    std::cout << "\n匹配:" << std::endl;
    for (auto i = 0; i < k_match.size(); ++i)
      std::cout << k_match[i].str() << std::endl;

    str_ep = k_match.suffix();
  }
}