#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <loggerlib/Logger.h>

#include <iostream>
#include <memory>

#include <corelib/core_Cpp.h>
#include <corelib/FileWarp/ImageSequence.h>
#include <corelib/FileWarp/VideoSequence.h>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>
#include <streambuf>

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
  doodle::FSys::path p_ue4_path;
};

void CoreTest::SetUp() {
  // auto prj = std::make_shared<doodle::Project>("W:/");
  // set.setProject(prj);

  p_maya_path = R"(D:\shot_ep016_sc0032_Anm_Animation_v0001_zhengshanshan.ma)";

  p_image_path = R"(D:\sc_064)";
  p_video_path = R"(D:\video)";

  p_video_path_out1 = R"(D:\voide\test1.mp4)";
  p_video_path_out2 = R"(D:\voide\test2.mp4)";
  p_txt_path        = R"(D:\test.txt)";
  p_ue4_path        = R"(F:\Users\teXiao\Documents\Unreal_Projects\test_tmp\test_tmp.uproject)";
}

void CoreTest::TearDown() {
}

TEST_F(CoreTest, archive) {
  auto& ue_set    = doodle::Ue4Setting::Get();
  auto str_stream = std::stringstream{};

  auto str_stream_bin = std::stringstream{};
  {
    cereal::JSONOutputArchive json{str_stream};
    json(cereal::make_nvp("mainset", set));
    // cereal::BinaryOutputArchive binary{std::cout};
    cereal::BinaryOutputArchive binary2{str_stream_bin};
    binary2(set);
  }

  std::cout << str_stream.str() << std::endl;
  // for (auto it = std::istream_iterator<char>(str_stream_bin);
  //      it == std::istream_iterator<char>();
  //      ++it) {
  //   std::cout << "\\x" << (*it) << " ";
  // }
  // std::cout << std::endl;
  ue_set.setVersion("4.26");

  {
    cereal::JSONInputArchive json{str_stream};
    json(set);
    cereal::BinaryInputArchive binary{str_stream_bin};
    binary(set);
  }
  std::cout
      << "\nuser : " << set.getUser()
      << "\nuser_en : " << set.getUser_en()
      << "\ndoc : " << set.getDoc()
      << "\ncacheRoot : " << set.getCacheRoot()
      << "\ndoc : " << set.getDepartment()
      << std::endl;
  std::cout
      << "\nue path: " << ue_set.Path()
      << "\nversipn: " << ue_set.Version()
      << "\nue shot start: " << ue_set.ShotStart()
      << "\nue shot end: " << ue_set.ShotEnd()
      << std::endl;
}

TEST_F(CoreTest, loadUe4ProjectFile) {
  doodle::FSys::ifstream file{p_ue4_path};
  auto str_stream = std::stringstream{};
  auto ijson      = nlohmann::json::parse(file);

  auto ueFile = ijson.get<doodle::Ue4ProjectFile>();
  ueFile.Plugins.push_back(doodle::Ue4ProjectFilePulgins{"doodle", true});
  nlohmann::json root = ueFile;
  // json(ueFile);
  std::cout
      << ijson
      << std::endl
      << root
      << std::endl;
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

TEST_F(CoreTest, shot_com) {
  auto shot1  = doodle::Shot{0};
  auto shot2  = doodle::Shot{2};
  auto shot3  = doodle::Shot{3};
  auto shot4  = doodle::Shot{4};
  auto shot11 = doodle::Shot{11};
  auto shot6  = doodle::Shot{6};
  auto shot6a = doodle::Shot{6, "a"};
  ASSERT_TRUE(shot1 < shot2);
  ASSERT_TRUE(shot6 < shot6a);
  ASSERT_TRUE(shot1 < shot11);
  ASSERT_TRUE(shot2 < shot11);
}