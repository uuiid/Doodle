#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <loggerlib/Logger.h>

#include <iostream>
#include <memory>

#include <corelib/core_Cpp.h>
#include <corelib/FileWarp/ImageSequence.h>
#include <corelib/FileWarp/VideoSequence.h>

class CoreTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  doodle::coreSet& set = doodle::coreSet::getSet();

  doodle::FSys::path p_maya_path;
  doodle::FSys::path p_image_path;
  doodle::FSys::path p_video_path;
  doodle::FSys::path p_video_path_out1;
  doodle::FSys::path p_video_path_out2;
  doodle::FSys::path p_txt_path;
};

void CoreTest::SetUp() {
  // auto prj = std::make_shared<doodle::Project>("W:/");
  // set.setProject(prj);

  p_maya_path  = R"(D:\shot_ep016_sc0032_Anm_Animation_v0001_zhengshanshan.ma)";
  p_image_path = R"(D:\sc_064)";
  p_video_path = R"(D:\video)";

  p_video_path_out1 = R"(D:\voide\test1.mp4)";
  p_video_path_out2 = R"(D:\voide\test2.mp4)";
  p_txt_path        = R"(D:\test.txt)";
}

void CoreTest::TearDown() {
}

TEST_F(CoreTest, getInfo) {
  std::cout
      << "\nuser : " << set.getUser()
      << "\nuser_en : " << set.getUser_en()
      << "\ndoc : " << set.getDoc()
      << "\ncacheRoot : " << set.getCacheRoot()
      << "\ndoc : " << set.getDepartment()
      << "\nsynpath : " << set.getSynPathLocale()
      << std::endl;
}

TEST_F(CoreTest, getProjectInfo) {
  auto prj = set.getProject();
  // auto path_parser = prj->findParser(rttr::type::get<doodle::assdepartment>());
  // std::cout
  //     << "\n name : " << prj->Name()
  //     << "\n root : " << prj->Root()
  //     << "\n doc : " << path_parser[0]
  //     << std::endl;
}

TEST_F(CoreTest, setInfo) {
}

TEST_F(CoreTest, export_maya) {
  auto mayafile = doodle::MayaFile{};
  mayafile.exportFbxFile(p_maya_path);
}

TEST_F(CoreTest, make_vide) {
  auto video = doodle::ImageSequence{p_image_path, {"test_哈哈"}};
  video.createVideoFile(p_video_path_out1);
}

TEST_F(CoreTest, connect_video) {
  auto videos = std::vector<doodle::FSys::path>{};
  for (auto v : doodle::FSys::directory_iterator(p_video_path)) {
    videos.emplace_back(v.path());
  }

  auto video = doodle::VideoSequence{videos};
  video.connectVideo(p_video_path_out2);
}

TEST_F(CoreTest, read_writ_file) {
  doodle::FSys::fstream file{p_txt_path, std::ios::in};
  std::string line{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

  static std::string str{R"("%ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache")"};
  auto it = line.find(str);
  while (it != std::string::npos) {
    // std::cout << line << std::endl;
    line.replace(it, str.size(), R"("%GAMEDIR%DerivedDataCache")");
    // std::cout << line << std::endl;
    it = line.find(str);
  }
  file.close();
  file.open(p_txt_path, std::ios::out | std::ios::trunc);
  file << line;
}