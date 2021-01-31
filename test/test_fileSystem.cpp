#include <fileSystem/fileSystem_cpp.h>

#include <loggerlib/Logger.h>

#include <QFileInfo>
#include <QDateTime>
#include <gtest/gtest.h>

#include <QDir>
#include <boost/filesystem/operations.hpp>

TEST(fileclient, uploadFile) {
  auto &file = doSystem::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/doodle.exe",
                          "/cache/tmp/Content/test.mp4", false));
}

TEST(fileclient, uploadFolder) {
  auto &file = doSystem::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/Users/teXiao/Documents/Unreal_Projects/test_fire_light/Content",
                          "/cache/tmp/Content", false));
}

TEST(fileclient, downFoluploadFele) {
  auto &file = doSystem::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp/1/volume_texture1.0001.exr", "/cache/test/volume_texture1.0001.exr", false));
}



TEST(fileclient, downFoluploadFolder) {
  auto &file = doSystem::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp", "/cache/test", false));
}

TEST(fileclient, exists) {
  auto &file = doSystem::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.exists("/cache/tmp/Content"));
}
