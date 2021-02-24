#include <corelib/filesystem/fileSync.h>
#include <corelib/filesystem/FileSystem.h>
#include <corelib/core/coreset.h>
#include <boost/locale.hpp>
#include <gtest/gtest.h>
#include <regex>
TEST(fileclient, uploadFile) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/doodle.exe",
                          "/cache/tmp/Content/test.mp4", false));
}

TEST(fileclient, uploadFileUTF8) {
  auto &set  = doodle::coreSet::getSet();
  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  auto soure  = boost::locale::conv::to_utf<char>("F:/测试文件.mp4", "UTF-8");
  auto trange = boost::locale::conv::to_utf<char>("/cache/tmp/Content/测试文件.mp4", "UTF-8");

  ASSERT_TRUE(file.upload(soure,
                          trange, false));
}

TEST(fileclient, uploadFolder) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.upload("F:/Users/teXiao/Documents/Unreal_Projects/test_fire_light/Content",
                          "/cache/tmp/Content", false));
}

TEST(fileclient, uploadFolderRegex) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  auto tmp = std::make_shared<doodle::fileDowUpdateOptions>();
  tmp->setlocaPath("F:/Users/teXiao/Documents/Unreal_Projects/test_fire_light/Content");
  tmp->setremotePath("/cache/tmp/Content");
  tmp->setInclude({std::make_shared<std::regex>("Shot")});
  tmp->setExclude({std::make_shared<std::regex>("Shot/shot_ep001_sc0009_")});

  ASSERT_TRUE(file.upload(tmp));
}

TEST(fileclient, downFoluploadFele) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp/1/volume_texture1.0001.exr",
                        "/cache/test/volume_texture1.0001.exr", false));
}

TEST(fileclient, downFoluploadFolder) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");

  ASSERT_TRUE(file.down("F:/Users/teXiao/Documents/Unreal_Projects/tmp",
                        R"(\03_Workflow\Assets\VFX\ep087\guixuehexindixue\Content\shot\ep087\sc0037)", false));
}

TEST(fileclient, downFoluploadFolderRegex) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.250", 6666, "", "", "test");
  auto tmp = std::make_shared<doodle::fileDowUpdateOptions>();
  tmp->setlocaPath("F:/Users/teXiao/Documents/Unreal_Projects/tmp");
  tmp->setremotePath("/cache/test");
  tmp->setInclude({std::make_shared<std::regex>(".*")});
  tmp->setExclude({std::make_shared<std::regex>("back*")});

  ASSERT_TRUE(file.down(tmp));
}

TEST(fileclient, exists) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.exists("/cache/tmp/Content"));
}

TEST(fileclient, copy) {
  auto &set = doodle::coreSet::getSet();

  auto &file = doodle::DfileSyntem::get();
  file.session("192.168.10.213", 6666, "", "", "test");
  ASSERT_TRUE(file.copy("/cache/tmp/Content", "/cache/tmp/Content2"));
}