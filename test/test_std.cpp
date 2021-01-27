#include <gtest/gtest.h>
#include <fstream>
#include <core_Cpp.h>
#include <Windows.h>

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

TEST(DSTD, readBigFile) {
  std::fstream file{};
  file.open("W:\\cache\\BiManMan_UE4.7z", std::ios::in | std::ios::out | std::ios::binary);

  std::string data{};
  const uint64_t off{8000000};
  data.resize(off);
  int64_t count{0};
  file.seekg(2152000000, std::ios::beg);
  file.read(data.data(), off);
  // for (size_t i = 0; i < 342; ++i) {
  //   if (!file.good()) break;
  // }

  if (file.fail())
    std::cout << "err : " << count << std::endl;
}