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

#include <boost/locale.hpp>
TEST(DSTD, map_netDir) {
  NETRESOURCE resources{};
  resources.dwType       = RESOURCETYPE_DISK;
  resources.lpLocalName  = "S:";
  resources.lpProvider   = 0;
  resources.lpRemoteName = R"(\\192.168.10.250\public\CangFeng)";
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
  std::cout << data.at(0) << std::endl;
  std::cout << data.data() << std::endl;
  const std::string data2{};
  // data2
}

TEST(DSTD, regex) {
  std::regex regex{"."};
}

TEST(DSTD, u8stringAndString) {
  std::filesystem::path str{"哈哈"};
  std::cout << str << std::endl;
  std::cout << str.generic_u8string() << std::endl;
  std::cout << str.generic_string() << std::endl;
  std::cout << "std::locale : " << std::locale{}.name().c_str() << std::endl;
  std::cout << typeid(std::filesystem::path).name() << std::endl;
  std::cout << typeid(std::cout).name() << std::endl;
}
#undef min
#undef max

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