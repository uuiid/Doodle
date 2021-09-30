//
// Created by TD on 2021/7/27.
//

#include <DoodleLib/doodle_lib_all.h>
#if defined(_WIN32)
#elif defined(__linux__)
#include <sys/stat.h>
#endif
#include <catch.hpp>

TEST_CASE("core pinyi", "[fun][pingyin]") {
  auto trs = doodle::convert::Get().toEn("aa大.?小d多dd53少");
  REQUIRE(trs == "aada.?xiaodduodd53shao");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("林奇");
  REQUIRE(trs == "linqi");
  std::cout << trs << std::endl;
  trs = doodle::convert::Get().toEn("李叶华");
  REQUIRE(trs == "liyehua");
  std::cout << trs << std::endl;
}

TEST_CASE("core fmt", "[fun][fmt]") {
  //  auto wstr = fmt::format(L"11{}", std::wstring{L"test"});
  //  REQUIRE(wstr == L"11test");
  //#if defined ( _WIN32 )
  //  const auto k_string = std::wstring{L"/还会"};
  //  wstr                = fmt::format(L"{}", doodle::FSys::path{k_string});
  //  REQUIRE(wstr == std::wstring{LR"("还会")"});
  //#elif defined ( __linux__ )
  //  const auto k_string = std::string{"/还会"};
  //  auto strl                = fmt::format("{}", doodle::FSys::path{k_string});
  //  REQUIRE(strl == std::string{R"("/还会")"});
  //#endif
  //  auto str = fmt::format("{:04d}", 2);
  //  REQUIRE(str == "0002");
  //  str = fmt::format("{}", doodle::FSys::path{"test"});
  //  REQUIRE(str == R"("test")");
}

TEST_CASE("core path ", "[fun][path]") {
  using namespace doodle;
#if defined(_WIN32)
  FSys::path source_path{L"F:/测试文件.mp4"};
  FSys::path root{"D:/"};
  FSys::path p1{L"D:\\9-houqi\\Content\\Character"};
#elif defined(__linux__)
  FSys::path p1{"/mnt/d/9-houqi/Content/Character"};
  FSys::path root{"/mnt/d/"};
  FSys::path source_path{"/mnt/f/测试文件.mp4"};
#endif

  SECTION("lexically_normal path") {
    FSys::path subdir("../configuration/instance");
    FSys::path cfgfile("../instance/myfile.cfg");
    root /= "tmp";
    root /= "test";
    root /= "ttt";
    FSys::path tmp(root / subdir / cfgfile);
    REQUIRE(tmp.generic_string() == FSys::path{"D:/tmp/test/ttt/../configuration/instance/../instance/myfile.cfg"}.generic_string());
    REQUIRE(tmp.lexically_normal() == FSys::path{"D:/tmp/test/configuration/instance/myfile.cfg"}.generic_string());
  }
  SECTION("file time") {
#if defined(_WIN32)
    struct _stat64 fileInfo {};
    if (_wstat64(source_path.generic_wstring().c_str(), &fileInfo) != 0) {
      std::cout << "Error : not find last_write_time " << std::endl;
    }
    auto k_t = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
#elif defined(__linux__)
    struct stat fileInfo {};
    stat(source_path.generic_string().c_str(), &fileInfo);
    auto k_t = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
#endif
    REQUIRE(k_t == FSys::last_write_time_point(source_path));
  }
  SECTION("folder time") {
#if defined(_WIN32)
    struct _stat64 fileInfo {};
    root /= "tmp";
    if (_wstat64(root.generic_wstring().c_str(), &fileInfo) != 0) {
      std::cout << "Error : not find last_write_time " << std::endl;
    }
    auto k_t = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
#elif defined(__linux__)
    struct stat fileInfo {};
    stat(root.generic_string().c_str(), &fileInfo);
#endif
    REQUIRE(k_t == FSys::last_write_time_point(root));
  }
  SECTION("file sys append") {
    auto k_path = source_path.replace_extension(".txt.backup");
    REQUIRE(k_path == FSys::path{"F:/测试文件.txt.backup"});
  }
  SECTION("path relative") {
    auto k_p = p1.root_name() / p1.root_directory() / p1.relative_path();
    REQUIRE(k_p == FSys::path{"D:/9-houqi/Content/Character"});
    REQUIRE(p1.root_path() == FSys::path{"D:/"});
    REQUIRE(p1.root_directory() == FSys::path{"/"});
    REQUIRE(p1.lexically_relative(root) == FSys::path{"9-houqi/Content/Character"});
  }
  SECTION("path open ex") {
    FSys::open_explorer(source_path.parent_path());
    FSys::open_explorer(root);
  }
}

TEST_CASE("core create_path", "[fun][create_path]") {
  using namespace doodle;

  auto k_1 = std::make_shared<project>("D:/", "ttt");
  auto k_2 = std::make_shared<assets>(k_1, "ttt");
  k_1->get_child().push_back(k_2);
  auto k_3 = std::make_shared<assets>(k_2, "eee");
  k_2->get_child().push_back(k_3);
  auto path = assets_path("D:/file/ex2.ma", k_3);
  SECTION("dir ") {
    REQUIRE(path.get_local_path() == FSys::path{"D:/file/ex2.ma"});
    REQUIRE(path.get_server_path() == FSys::path{"ttt/ttt/eee/ex2.ma"});
  }
  SECTION("ip path") {
    k_1->set_path("//192.168.10.250/public/changanhuanjie");
    REQUIRE(path.get_local_path() == FSys::path{"D:/file/ex2.ma"});
    REQUIRE(path.get_server_path() == FSys::path{"ttt/ttt/eee/ex2.ma"});
  }
  SECTION("ip path2") {
    auto path2 = assets_path("//192.168.10.250/public/changanhuanjie/5-moxing/Ch/Ch001A/Rig/Ch001A_Rig_lyq.ma", k_3);
    k_1->set_path("//192.168.10.250/public/changanhuanjie");
    REQUIRE(path2.get_local_path() == FSys::path{"//192.168.10.250/public/changanhuanjie/5-moxing/Ch/Ch001A/Rig/Ch001A_Rig_lyq.ma"});
    REQUIRE(path2.get_server_path() == FSys::path{"ttt/ttt/eee/Ch001A_Rig_lyq.ma"});
  }
}

#include <boost/locale/generator.hpp>
#include <opencv2/opencv.hpp>
TEST_CASE("core opencv", "[fun]") {
  using namespace doodle;
  const FSys::path path{R"(D:\tmp\tmp)"};
  std::fstream file{};
  int i      = 0;

  auto video = cv::VideoWriter("D:/test.mp4", cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), (double)25, cv::Size{1280, 720});
  cv::Mat image{};
  for (auto&& it_p : FSys::directory_iterator(path)) {
    if (it_p.is_regular_file()) {
      image = cv::imread(it_p.path().string());
      video << image;
    }
  }
}
TEST_CASE("core opencv image", "[fun]") {
  const std::filesystem::path path{R"(D:\image_test\DB0106_sc0078_V.1001.png)"};
  auto k_mat = cv::imread(path.generic_string());
  auto k_gbr = k_mat.row(1).col(1);
  std::cout
      << "1,1 bgr-> " << k_gbr << "\n"
      << "info: " << k_mat.type() << "\n"
      << "channels: " << k_mat.channels() << "\n"
      << "elemSize: " << k_mat.elemSize() << "\n"
      << "elemSize1: " << k_mat.elemSize1() << "\n"
      << "step: " << k_mat.step << "\n"
      << "step1: " << k_mat.step1() << "\n"
      << std::endl;
}

TEST_CASE("maya get log", "[maya]") {
  using namespace doodle;
  auto k_maya = maya_file();
  auto k_arg  = std::make_shared<maya_file::qcloth_arg>();
  //  k_arg->only_sim           = false;
  //  k_arg->qcloth_assets_path = FSys::path{R"(V:\03_Workflow\Assets\CFX\cloth)"};
  //  k_arg->sim_path           = FSys::path{"F:\\data\\DBXY_163_052.ma"};
  //  auto k_term               = k_maya.qcloth_sim_file(k_arg);
  //  k_term->sig_message_result.connect([](const std::string& in_, long_term::level in_level) { DOODLE_LOG_INFO(in_); });
  //  k_term->sig_progress.connect([](auto in_) { DOODLE_LOG_INFO(in_); });
  //  k_term->p_list[0].get();
}

TEST_CASE("ThreadPool", "[core][ThreadPool]") {
  using namespace doodle;
  thread_pool th{4};
  for (int k_i = 0; k_i < 100; ++k_i) {
    th.enqueue([k_i]() {
      std::cout << k_i << std::endl;
    });
  }
}

#include <boost/locale.hpp>
#include <boost/locale/info.hpp>
#include <boost/locale/util.hpp>
TEST_CASE("sys encoding", "[sys]") {
  auto k_ = boost::locale::util::get_system_locale();
  std::cout << k_ << std::endl;
  std::cout << boost::locale::conv::from_utf<char>("测试", "windows-936") << std::endl;

  auto k_lo = boost::locale::generator()("");
  std::cout << std::use_facet<boost::locale::info>(k_lo).encoding() << std::endl;
  std::cout << std::use_facet<boost::locale::info>(k_lo).name() << std::endl;
  std::cout << std::use_facet<boost::locale::info>(k_lo).variant() << std::endl;
  //  std::cout << k_.c_str() << std::endl;
}

#include <boost/rational.hpp>

TEST_CASE("boost rational", "[boost][rational]") {
  using rati = boost::rational<std::uint64_t>;
  auto k_i   = rati(1, 5);
  auto k_i2  = rati(1, 10);
  std::cout << boost::rational_cast<std::double_t>(((k_i + k_i2) / rati{5}) * rati{2}) << std::endl;
}

TEST_CASE("std regex", "[std][regex]") {
  std::cout.setf(std::ios_base::boolalpha);
  //致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
  const static std::wregex fatal_error_znch{
      LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma 中保存)"};

  // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
  const static std::wregex fatal_error_en_us{
      LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma)"};

  std::cout << std::regex_search(L"致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存",
                                 fatal_error_znch)
            << std::endl;
  std::cout << std::regex_search(L"Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma",
                                 fatal_error_en_us)
            << std::endl;
}

TEST_CASE("core archive", "[fun][archives]") {
  using namespace doodle;
  //  auto& set           = CoreSet::getSet();
  //  auto& ue_set        = Ue4Setting::Get();
  auto str_stream     = std::stringstream{};
  auto str_stream_bin = std::stringstream{};
  SECTION("archive") {
    doodle::metadata_ptr k_val = std::make_shared<doodle::project>("D:/", "test22333");
    {
      boost::archive::text_oarchive json{str_stream};
//      boost::archive::xml_oarchive xml{str_stream_bin};
      //      boost::archive::polymorphic_text_oarchive json{str_stream};
      json << boost::serialization::make_nvp("metadata1", k_val);
      //      xml << boost::serialization::make_nvp("mainset", k_val);
      // cereal::BinaryOutputArchive binary{std::cout};
      //      cereal::BinaryOutputArchive binary2{str_stream_bin};
      //      binary2(k_val);
    }
    std::cout << str_stream.str() << std::endl;
    std::cout << str_stream_bin.str() << std::endl;
    k_val.reset();
    {
      boost::archive::text_iarchive json{str_stream};
//      boost::archive::xml_iarchive xml{str_stream_bin};
      //      boost::archive::polymorphic_text_iarchive json{str_stream};
      json >> k_val;
      //      xml >> k_val;
      //      cereal::BinaryInputArchive binary{str_stream_bin};
      //      binary(k_val);
    }
    // REQUIRE(k_val->getPath() == FSys::path{"D:/"});
    // REQUIRE(k_val->getName() == FSys::path{"test22333"});
    REQUIRE(k_val->show_str() == FSys::path{"test22333"});
    str_stream.clear();
    str_stream_bin.clear();
    SECTION("archive polymorphism") {
      {
        doodle::metadata_ptr k_m1 = std::make_shared<doodle::project>("D:/", "测试1");
        doodle::metadata_ptr k_m2 = std::make_shared<doodle::project>("F:/", "测试2");
        boost::archive::text_oarchive json{str_stream};
        //        boost::archive::polymorphic_text_oarchive json{str_stream};
        json << boost::serialization::make_nvp("metadata1", k_m1) << boost::serialization::make_nvp("metadata12", k_m2);
        // cereal::BinaryOutputArchive binary{std::cout};
        //        cereal::BinaryOutputArchive binary2{str_stream_bin};
        //        binary2(boost::serialization::make_nvp("metadata1", k_m1),
        //                boost::serialization::make_nvp("metadata12", k_m2));
      }
      std::cout << str_stream.str() << std::endl;
      std::cout << str_stream_bin.str() << std::endl;
      {
        doodle::metadata_ptr k1;
        doodle::metadata_ptr k2;

        boost::archive::text_iarchive json{str_stream};
        //        boost::archive::polymorphic_text_iarchive json{str_stream};
        json >> k1 >> k2;
        //        cereal::BinaryInputArchive binary{str_stream_bin};
        //        binary(k1, k2);
        REQUIRE(std::dynamic_pointer_cast<project>(k1)->get_path() == FSys::path{"D:/"});
        REQUIRE(std::dynamic_pointer_cast<project>(k1)->get_name() == "测试1");
        REQUIRE(std::dynamic_pointer_cast<project>(k2)->get_path() == FSys::path{"F:/"});
        REQUIRE(std::dynamic_pointer_cast<project>(k2)->get_name() == "测试2");
      }
    }
  }
}

TEST_CASE("temp fun", "[core]") {
  using namespace doodle;

  auto ter = new_object<long_term>();
  REQUIRE(doodle_lib::Get().long_task_list.size() == 1);
}
//#include <boost/algorithm/string.hpp>
//#include <boost/archive/iterators/base64_from_binary.hpp>
//#include <boost/archive/iterators/binary_from_base64.hpp>
//#include <boost/archive/iterators/transform_width.hpp>
// std::string encode64(const std::string &val) {
//  using namespace boost::archive::iterators;
//  using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
//  auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
//  return tmp.append((3 - val.size() % 3) % 3, '=');
//}
// std::string decode64(const std::string &val) {
//  using namespace boost::archive::iterators;
//  using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
//  return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
//    return c == '\0';
//  });
//}
