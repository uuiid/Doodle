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