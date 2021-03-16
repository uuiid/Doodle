#include <gtest/gtest.h>
#include <fstream>
#include <corelib/core_Cpp.h>
#include <Windows.h>
#include <iostream>
#include <locale>
#include <regex>

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