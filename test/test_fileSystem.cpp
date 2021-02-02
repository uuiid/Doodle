#include <corelib/filesystem/fileSync.h>
#include <corelib/filesystem/FileSystem.h>
#include <gtest/gtest.h>
#include <regex>
TEST(fileclient, uploadFile) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/doodle.exe",
                          "/cache/tmp/Content/test.mp4", false));
}

TEST(fileclient, uploadFolder) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/Users/teXiao/Documents/Unreal_Projects/test_fire_light/Content",
                          "/cache/tmp/Content", false));
}

TEST(fileclient, downFoluploadFele) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp/1/volume_texture1.0001.exr", "/cache/test/volume_texture1.0001.exr", false));
}

TEST(fileclient, downFoluploadFolder) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp", "/cache/test", false));
}

TEST(fileclient, downFoluploadFolderRegex) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  auto tmp = std::make_shared<doodle::fileDowUpdateOptions>();
  tmp->setlocaPath("F:/Users/teXiao/Documents/Unreal_Projects/tmp");
  tmp->setremotePath("/cache/test");
  tmp->setInclude({std::regex(".*")});
  tmp->setExclude({std::regex("back*")});

  ASSERT_TRUE(file.down(tmp));
}

TEST(fileclient, exists) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.exists("/cache/tmp/Content"));
}

TEST(fileclient, copy) {
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.copy("/cache/tmp/Content", "/cache/tmp/Content2"));
}