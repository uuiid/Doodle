#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <memory>

#include <DoodleLib/DoodleLib.h>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/binary.hpp>

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
  doodle::FSys::path p_long_path;
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
  p_long_path       = R"(F:\Users\teXiao\Documents\Unreal_Projects\test_tmp\Content\Dev\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\test_long_path\NewMaterial.uasset)";
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

    doodle::MetadataSet::Get().installProject(std::make_shared<doodle::Project>(
        "D:/", "test22333"));
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

TEST_F(CoreTest, load_save_meatdata) {
  using namespace doodle;
  auto k_f = std::make_shared<MetadataFactory>();
  {  //创建项目各各种标签
    auto ptj = std::make_shared<Project>("D:/", "test_23333");

    ptj->save(k_f);
    auto eps = std::make_shared<Episodes>(ptj, 10);
    eps->SetPParent(ptj);
    eps->save(k_f);
    for (auto i = 0; i < 100; ++i) {
      auto shot = std::make_shared<Shot>(eps, i);
      shot->SetPParent(eps);
      shot->save(k_f);
    }
    auto k_ass = std::make_shared<Assets>(ptj, "tset");
    k_ass->SetPParent(ptj);
    k_ass->save(k_f);
    auto k_ass_file = std::make_shared<AssetsFile>(k_ass, "tset", "测试");
    k_ass_file->SetPParent(k_ass);
    k_ass_file->save(k_f);
  }

  {
    //加载文件
    auto ptj = std::make_shared<Project>("D:/");
    ptj->load(k_f);
    std::cout << ptj->ShowStr() << std::endl;
    for (const auto& it : ptj->GetPChildItems()) {
      std::cout << " |>" << it->ShowStr() << std::endl;
      for (const auto& it1 : it->GetPChildItems()) {
        std::cout << "   |>" << it1->ShowStr() << std::endl;
      }
    }
  }
}

TEST_F(CoreTest, loadUe4ProjectFile) {
  ASSERT_TRUE(doodle::FSys::exists(p_ue4_path));

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
  ASSERT_TRUE(doodle::FSys::exists(p_maya_path));
  auto mayafile = doodle::MayaFile{};
  mayafile.exportFbxFile(p_maya_path);
}

TEST_F(CoreTest, make_vide) {
  ASSERT_TRUE(doodle::FSys::exists(p_image_path));
  auto video = doodle::ImageSequence{p_image_path, {"test_哈哈"}};
  video.createVideoFile(p_video_path_out1);
}

TEST_F(CoreTest, connect_video) {
  ASSERT_TRUE(doodle::FSys::exists(p_video_path));
  auto videos = std::vector<doodle::FSys::path>{};
  for (auto v : doodle::FSys::directory_iterator(p_video_path)) {
    videos.emplace_back(v.path());
  }

  auto video = doodle::VideoSequence{videos};
  video.connectVideo(p_video_path_out2);
}

TEST_F(CoreTest, read_writ_file) {
  ASSERT_TRUE(doodle::FSys::exists(p_txt_path));
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
TEST_F(CoreTest, long_path) {
  using namespace doodle;
  ASSERT_TRUE(FSys::exists(p_long_path));

  auto size            = FSys::file_size(p_long_path);
  auto last_write_time = FSys::last_write_time(p_long_path);
  auto time            = FSys::creation_time(p_long_path);
  FSys::fstream file{p_long_path};
  std::cout << "file size " << size << "\n"
            << "lase write time " << last_write_time << "\n"
            << "create time " << time << std::endl
            << (file.is_open() ? "true" : "false") << std::endl;
}
