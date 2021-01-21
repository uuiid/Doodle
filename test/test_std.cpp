#include <gtest/gtest.h>

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