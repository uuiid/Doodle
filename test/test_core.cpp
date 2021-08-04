#include <DoodleLib/DoodleLib.h>
#include <date/date.h>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <iostream>
#include <memory>
#include <streambuf>
class CoreTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  doodle::CoreSet& set = doodle::CoreSet::getSet();

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
  DoodleLib::Get().init_gui();
  p_maya_path = LR"(D:/shot_ep016_sc0032_Anm_Animation_v0001_zhengshanshan.ma)";

  p_image_path = LR"(D:/sc_064)";
  p_video_path = LR"(D:/video)";

  p_video_path_out1 = LR"(D:/voide/test1.mp4)";
  p_video_path_out2 = LR"(D:/voide/test2.mp4)";
  p_txt_path        = LR"(D:/test.txt)";
  p_ue4_path        = LR"(F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/test_tmp.uproject)";
  p_long_path       = LR"(F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/Content/Dev/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/test_long_path/NewMaterial.uasset)";
}

void CoreTest::TearDown() {
  set.clear();
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
    doodle::DoodleLib::Get().p_project_vector.push_back_sig(std::make_shared<doodle::Project>(
        "D:/", "test22333"));

    binary2(set);
  }

  std::cout << str_stream.str() << std::endl;
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

TEST_F(CoreTest, archive_polymorphism) {
  auto str_stream = std::stringstream{};

  auto k_m                 = std::make_shared<doodle::Project>("D:/", "测试1");
  doodle::MetadataPtr k_m2 = std::make_shared<doodle::Project>("D:/", "测试2");

  auto str_stream_bin = std::stringstream{};
  {
    cereal::JSONOutputArchive json{str_stream};
    json(cereal::make_nvp("metadata1", k_m),
         cereal::make_nvp("metadata12", k_m2));
    // cereal::BinaryOutputArchive binary{std::cout};
    cereal::BinaryOutputArchive binary2{str_stream_bin};
    binary2(cereal::make_nvp("metadata1", k_m),
            cereal::make_nvp("metadata12", k_m2));
  }

  std::cout << str_stream.str() << std::endl;

  {
    doodle::ProjectPtr k1;
    doodle::ProjectPtr k2;

    cereal::JSONInputArchive json{str_stream};
    json(k1, k2);
    cereal::BinaryInputArchive binary{str_stream_bin};
    binary(k1, k2);
  }
}

TEST_F(CoreTest, create_meatdata) {
  using namespace doodle;
  std::string k_test_root{};
  auto k_f = std::make_shared<MetadataFactory>();
  {  //创建项目各各种标签
    auto ptj = std::make_shared<Project>("D:/", "test_23333");
    ptj->updata_db(k_f);

    ASSERT_TRUE(ptj->getMetadataFactory() == k_f);
    DoodleLib::Get().p_project_vector.push_back_sig(ptj);

    for (auto i = 1; i <= 10; ++i) {
      switch (i) {
        case 1: {
          auto k_ass = std::make_shared<Assets>(ptj, "tset");

          ptj->child_item.push_back_sig(k_ass);
          k_ass->updata_db(k_f);
          ASSERT_TRUE(k_ass->getMetadataFactory() == k_f);
          k_ass = std::make_shared<Assets>(ptj, "test_m_parent");
          ptj->child_item.push_back_sig(k_ass);
          k_ass->updata_db(k_f);
          ASSERT_TRUE(k_ass->getMetadataFactory() == k_f);
          ASSERT_TRUE(k_ass->getParent() == ptj);
          k_test_root = k_ass->getUUID();

          auto k_ass_file = std::make_shared<AssetsFile>(k_ass,
                                                         "tset",
                                                         "测试");
          k_ass->child_item.push_back_sig(k_ass_file);
          k_ass_file->updata_db(k_f);
          ASSERT_TRUE(k_ass_file->getMetadataFactory() == k_f);
          continue;
        } break;

        default: {
          auto eps = std::make_shared<Episodes>(ptj, i);
          ptj->child_item.push_back_sig(eps);
          eps->updata_db(k_f);
          for (auto x = 1; x < 30; ++x) {
            auto shot = std::make_shared<Shot>(eps, x);
            eps->child_item.push_back_sig(shot);
            shot->updata_db(k_f);
            if (i % 5 == 0) {
              shot->setShotAb(Shot::ShotAbEnum::A);
            }
            ASSERT_TRUE(shot->getMetadataFactory() == k_f);
          }
        } break;
      }
    }
    std::cout << "prj ch size: " << ptj->child_item.size() << std::endl;
    ptj->updata_db(k_f);
    set.writeDoodleLocalSet();
  }
}


TEST_F(CoreTest, loadUe4ProjectFile) {
  ASSERT_TRUE(doodle::FSys::exists(p_ue4_path));

  doodle::FSys::ifstream file{p_ue4_path};
  auto str_stream = std::stringstream{};
  auto ijson      = nlohmann::json::parse(file);

  // auto ueFile = ijson.get<doodle::Ue4ProjectFile>();
  // ueFile.Plugins.push_back(doodle::Ue4ProjectFilePulgins{"doodle", true});
  // nlohmann::json root = ueFile;
  // // json(ueFile);
  // std::cout
  //     << ijson
  //     << std::endl
  //     << root
  //     << std::endl;
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
  auto last_write_time = FSys::last_write_time_t(p_long_path);

  FSys::fstream file{p_long_path};
  std::cout << "file size " << size << "\n"
            << "lase write time " << last_write_time << "\n"
            << (file.is_open() ? "true" : "false") << std::endl;
}

TEST_F(CoreTest, actn_excel) {
}

TEST(core, time) {
  using namespace doodle;
  using namespace doodle::chrono::literals;
  auto k_time1     = TimeDuration{};
  auto k_sys_time1 = chrono::local_days(2020_y / 7 / 21_d) + 10h + 45min + 30s;
  auto k_sys_time2 = chrono::local_days(2020_y / 7 / 23_d) + 16h + 20min + 30s;
  k_time1.set_local_time(k_sys_time1);
  std::cout << k_time1.showStr() << std::endl;
  auto k_time2 = TimeDuration{};
  k_time2.set_local_time(k_sys_time2);
  std::cout << k_time2.showStr() << std::endl;
  std::cout << k_time1.work_duration(k_time2).count() << std::endl;
  /// 测试待周末的
  k_sys_time1 = chrono::local_days(2020_y / 7 / 21_d) + 10h + 45min + 30s;
  k_sys_time2 = chrono::local_days(2020_y / 7 / 27_d) + 16h + 20min + 30s;
  k_time1.set_local_time(k_sys_time1);
  std::cout << k_time1.showStr() << std::endl;
  k_time2.set_local_time(k_sys_time2);
  std::cout << k_time2.showStr() << std::endl;
  std::cout << k_time1.work_duration(k_time2).count() << std::endl;
}
