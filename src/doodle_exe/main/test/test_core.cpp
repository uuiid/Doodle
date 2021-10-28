//
// Created by TD on 2021/7/27.
//

#include <doodle_lib/doodle_lib_all.h>
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
  auto str = fmt::format("{:04d}", 2);
  REQUIRE(str == "0002");
  str = fmt::format("{}", doodle::FSys::path{"test"});
  REQUIRE(str == R"("test")");
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
  auto k_prj = make_handle(g_reg()->create());
  k_prj.emplace<project>("D:/", "ttt");
  auto k_ass = make_handle(g_reg()->create());
  k_ass.emplace<assets>("assets");
  k_ass.get<tree_relationship>().set_parent(k_prj);

  auto k_ass2 = make_handle(g_reg()->create());
  k_ass2.emplace<assets>("eee");
  k_ass2.get<tree_relationship>().set_parent(k_ass);

  auto k_ass_file = make_handle(g_reg()->create());
  k_ass_file.emplace<assets_file>("eee");
  k_ass_file.get<tree_relationship>().set_parent(k_ass2);

  auto& k_p = k_ass_file.emplace<assets_path>();
  SECTION("dir") {
    k_p.set_path("E:/tmp/ref/Gumu_rig_cloth.ma", k_ass2);
    std::cout << k_p.get_local_path() << std::endl;
    std::cout << k_p.get_server_path() << std::endl;
    REQUIRE(k_p.get_local_path() == FSys::path{"E:/tmp/ref/Gumu_rig_cloth.ma"});
    REQUIRE(k_p.get_server_path() == FSys::path{"ttt\\assets/eee/None_/Gumu_rig_cloth.ma"});
  }
  SECTION("ip path") {
    k_p.set_path("//192.168.10.250/public/changanhuanjie", k_ass2);
    std::cout << k_p.get_local_path() << std::endl;
    std::cout << k_p.get_server_path() << std::endl;
    REQUIRE(k_p.get_local_path() ==
            FSys::path{"//192.168.10.250/public/changanhuanjie"});
    REQUIRE(k_p.get_server_path() ==
            FSys::path{"ttt\\assets\\eee\\None_\\changanhuanjie"});
  }
  SECTION("ip path2") {
    k_p.set_path("//192.168.10.250/public/changanhuanjie/5-moxing/Ch/Ch001A/Rig/Ch001A_Rig_lyq.ma", k_ass2);
    std::cout << k_p.get_local_path() << std::endl;
    std::cout << k_p.get_server_path() << std::endl;
    REQUIRE(k_p.get_local_path() ==
            FSys::path{"//192.168.10.250/public/changanhuanjie/5-moxing/Ch/Ch001A/Rig/Ch001A_Rig_lyq.ma"});
    REQUIRE(k_p.get_server_path() ==
            FSys::path{"ttt\\assets\\eee\\None_\\Ch001A_Rig_lyq.ma"});
  }
  SECTION("ue4_file") {
    FSys::path l_ue{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/test_tmp.uproject"};
    auto& k_pv = k_ass_file.emplace<assets_path_vector>();
    SECTION("not use repath") {
      k_pv.add_file(l_ue);
      std::cout << " k_pv.paths.front().get_local_path()  :" << k_pv.paths.front().get_local_path() << std::endl;
      std::cout << " k_pv.paths.front().get_server_path() :" << k_pv.paths.front().get_server_path() << std::endl;
      std::cout << " k_pv.paths.front().get_backup_path() :" << k_pv.paths.front().get_backup_path() << std::endl;
      std::cout << " k_pv.paths.front().get_cache_path()  :" << k_pv.paths.front().get_cache_path() << std::endl;
      REQUIRE(k_pv.paths.front().get_local_path() ==
              FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/test_tmp.uproject"});
      REQUIRE(k_pv.paths.front().get_server_path() ==
              FSys::path{"ttt\\assets\\eee\\None_\\test_tmp.uproject"});
      std::cout << " k_pv.paths[1].get_local_path()  :" << k_pv.paths[1].get_local_path() << std::endl;
      std::cout << " k_pv.paths[1].get_server_path() :" << k_pv.paths[1].get_server_path() << std::endl;
      REQUIRE(k_pv.paths[1].get_local_path() ==
              FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/Content"});
      REQUIRE(k_pv.paths[1].get_server_path() ==
              FSys::path{"ttt\\assets\\eee\\None_\\Content"});
    }
    SECTION("using repath") {
      SECTION("root not eq") {
        k_pv.add_file(l_ue, true);
        std::cout << " k_pv.paths.front().get_local_path()  :" << k_pv.paths.front().get_local_path() << std::endl;
        std::cout << " k_pv.paths.front().get_server_path() :" << k_pv.paths.front().get_server_path() << std::endl;
        REQUIRE(k_pv.paths.front().get_local_path() ==
                FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/test_tmp.uproject"});
        REQUIRE(k_pv.paths.front().get_server_path() ==
                FSys::path{"ttt\\test_tmp.uproject"});
        std::cout << " k_pv.paths[1].get_local_path()  :" << k_pv.paths[1].get_local_path() << std::endl;
        std::cout << " k_pv.paths[1].get_server_path() :" << k_pv.paths[1].get_server_path() << std::endl;
        REQUIRE(k_pv.paths[1].get_local_path() ==
                FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/Content"});
        REQUIRE(k_pv.paths[1].get_server_path() ==
                FSys::path{"ttt\\Content"});
      }
      SECTION("root eq") {
        k_prj.get<project>().set_path("F:/");
        k_pv.add_file(l_ue, true);
        std::cout << " k_pv.paths.front().get_local_path()  :" << k_pv.paths.front().get_local_path() << std::endl;
        std::cout << " k_pv.paths.front().get_server_path() :" << k_pv.paths.front().get_server_path() << std::endl;
        REQUIRE(k_pv.paths.front().get_local_path() ==
                FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/test_tmp.uproject"});
        REQUIRE(k_pv.paths.front().get_server_path() ==
                FSys::path{"ttt\\Users\\teXiao\\Documents\\Unreal_Projects\\test_tmp\\test_tmp.uproject"});
        std::cout << " k_pv.paths[1].get_local_path()  :" << k_pv.paths[1].get_local_path() << std::endl;
        std::cout << " k_pv.paths[1].get_server_path() :" << k_pv.paths[1].get_server_path() << std::endl;
        REQUIRE(k_pv.paths[1].get_local_path() ==
                FSys::path{"F:/Users/teXiao/Documents/Unreal_Projects/test_tmp/Content"});
        REQUIRE(k_pv.paths[1].get_server_path() ==
                FSys::path{"ttt\\Users\\teXiao\\Documents\\Unreal_Projects\\test_tmp\\Content"});
      }
    }
  }
}

#include <boost/locale/generator.hpp>
#include <opencv2/opencv.hpp>
TEST_CASE("core opencv", "[fun]") {
  using namespace doodle;
  const FSys::path path{R"(D:\tmp\image_test_ep002_sc001)"};
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
  for (int k_i = 0; k_i < 10; ++k_i) {
    doodle_lib::Get().get_thread_pool()->enqueue([k_i]() {
      std::cout << k_i << std::endl;
      return;
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
  REQUIRE(std::regex_search(L"致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存",
                            fatal_error_znch));
  std::cout << std::regex_search(L"Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma",
                                 fatal_error_en_us)
            << std::endl;
  REQUIRE(std::regex_search(L"Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma",
                            fatal_error_en_us));
}

TEST_CASE("temp fun", "[core]") {
  using namespace doodle;

  auto ter = new_object<long_term>();
  REQUIRE(doodle_lib::Get().long_task_list.size() == 1);
}

TEST_CASE("path iter", "[core]") {
  using namespace doodle;
  FSys::path k_path{"Users\\teXiao\\Documents\\Unreal_Projects\\test_tmp\\test_tmp.uproject"};
  for (auto& i : k_path) {
    std::cout << i << std::endl;
  }
}
TEST_CASE("image sequence", "[core]") {
  using namespace doodle;

  std::vector<FSys::path> p_list{
      FSys::directory_iterator{"D:\\tmp\\image_test_sc001"},
      FSys::directory_iterator{}};

  std::cout << fmt::format("{}", fmt::join(p_list, " \n")) << std::endl;
  REQUIRE(image_sequence::is_image_sequence(p_list));
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
