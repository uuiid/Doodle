//
// Created by TD on 2021/7/27.
//

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/exe_warp/maya_exe.h>

#include <doodle_lib/doodle_lib_all.h>
#if defined(_WIN32)
#elif defined(__linux__)
#include <sys/stat.h>
#endif
#include <catch.hpp>

#include <doodle_lib/app/app.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/gui/gui_ref/cross_frame_check.h>

using namespace doodle;

class test_pinyin {
 public:
  std::string data{"aa大.?小d多dd53少"};
  std::string data2{"林奇"};
  std::string data3{"李叶华"};
};

class test_regex {
 public:
  std::string regex_{R"(\{(\w+)(:[^{}]?[<>^]?[+-]?#?0?(?:\d+|(\{\w+\}))?\.?(?:\d+|(\{\w+\}))?L?[\w]?)\})"};
  std::string str1_{"dsad_{eps:d<+#0{asd}.{asd}Ld}_{shot:d<+#{asd}.{asd}Ld}_{end:.{asd}Ld}_{start:d<{asd}.{asd}d}"};
  //  std::string str2{""};
  //  std::string str3{""};
};

TEST_CASE_METHOD(test_regex, "test_regex_1", "[core][regex]") {
  std::regex k_reg{regex_};
  for (std::sregex_iterator i = std::sregex_iterator(str1_.begin(), str1_.end(), k_reg);
       i != std::sregex_iterator();
       ++i) {
    std::smatch m = *i;
    std::cout << '\n';
    std::cout << " Match size: " << m.size() << '\n';
    std::cout << "Match value: " << m.str() << " at Position " << m.position() << '\n';
    std::cout << "    Capture: " << m[1].str() << " at Position " << m.position(1) << '\n';
    std::cout << "     Format: " << m.format("$` <<< $& >>>  $'") << '\n';
  }
}

struct point {
  double x, y;
};

template <>
struct fmt::formatter<point> {
  // Presentation format: 'f' - fixed, 'e' - exponential.
  char presentation = 'f';

  // Parses format specifications of the form ['f' | 'e'].
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    // [ctx.begin(), ctx.end()) is a character range that contains a part of
    // the format string starting from the format specifications to be parsed,
    // e.g. in
    //
    //   fmt::format("{:f} - point of interest", point{1, 2});
    //
    // the range will contain "f} - point of interest". The formatter should
    // parse specifiers until '}' or the end of the range. In this example
    // the formatter should parse the 'f' specifier and return an iterator
    // pointing to '}'.

    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}')
      throw format_error("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the point p using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const point& p, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(
        ctx.out(),
        presentation == 'f' ? "({:.1f}, {:.1f})" : "({:.1e}, {:.1e})",
        p.x, p.y
    );
  }
};

TEST_CASE_METHOD(test_pinyin, "core pin_yi", "[fun][pingyin]") {
  auto trs = doodle::convert::Get().toEn(data);
  REQUIRE(trs == "aada.?xiaodduodd53shao");
  INFO(trs);
  trs = doodle::convert::Get().toEn(data2);
  REQUIRE(trs == "linqi");
  INFO(trs);
  trs = doodle::convert::Get().toEn(data3);
  INFO(trs);
  REQUIRE(trs == "liyehua");
}

TEST_CASE("core fmt", "[fun][fmt]") {
  auto str = fmt::format("{:04d}", 2);
  REQUIRE(str == "0002");
  str = fmt::format("{}", doodle::FSys::path{"test"});
  REQUIRE(str == R"(test)");

  REQUIRE(fmt::to_string(doodle::episodes{1}) == R"(ep_1)");
  point p = {1, 2};
  str     = fmt::format("{:f}", p);
  INFO(str);

  INFO(fmt::to_string(episodes{1}));
}

class test_path_ {
 public:
  FSys::path source_path{L"F:/测试文件.mp4"};
  FSys::path root{"D:/"};
  FSys::path p1{L"D:\\9-houqi\\Content\\Character"};

  FSys::path p_1{"das/ddd/1111"};
  FSys::path p_2{"das/ddd"};
  FSys::path p_3{"das/ddd/"};
};

class test_path_2 : public test_path_ {
 public:
  FSys::path subdir{"../configuration/instance"};
  FSys::path cfgfile{"../instance/myfile.cfg"};
};
class test_hash {
 public:
  constexpr static std::uint32_t class_hash() {
    return 1;
  }
};

TEST_CASE("test entt hash") {
  std::cout << entt::type_id<test_hash>().hash() << std::endl;
  std::cout << entt::type_id<project>().hash() << std::endl;
  std::cout << entt::type_id<episodes>().hash() << std::endl;
  REQUIRE(entt::type_id<episodes>().hash() == "class doodle::episodes"_hs);
  REQUIRE(648452978 == "class doodle::project::cloth_config"_hs);
}

TEST_CASE_METHOD(test_path_2, "test ranges") {
  std::regex l_regex{"UE4"};

  auto it = ranges::find_if(ranges::make_subrange(FSys::directory_iterator{root / "tmp"}, FSys::directory_iterator{}), [&](const FSys::path& in_file) {
    auto&& l_ext = in_file.extension();
    std::cout << in_file << std::endl;
    return (l_ext == ".png" || l_ext == ".jpg") && std::regex_search(in_file.filename().generic_string(), l_regex);
  });
  if (it != FSys::directory_iterator{})
    std::cout << (*it).path();
}

TEST_CASE_METHOD(test_path_2, "test_path_1", "[fun][path]") {
  root /= "tmp";
  root /= "test";
  root /= "ttt";
  FSys::path tmp(root / subdir / cfgfile);
  REQUIRE(tmp.generic_string() == FSys::path{"D:/tmp/test/ttt/../configuration/instance/../instance/myfile.cfg"}.generic_string());
  REQUIRE(tmp.lexically_normal() == FSys::path{"D:/tmp/test/configuration/instance/myfile.cfg"}.generic_string());
}

TEST_CASE_METHOD(test_path_, "test 比较") {
  std::cout << std::boolalpha
            << "(p_1 > p_2): " << (p_1 > p_2) << "\n"
            << "(p_1 > p_3): " << (p_1 > p_3);
}

TEST_CASE_METHOD(test_path_, "test_path_2_time", "[fun][path]") {
  struct _stat64 fileInfo {};
  if (_wstat64(source_path.generic_wstring().c_str(), &fileInfo) != 0) {
    std::cout << "Error : not find last_write_time " << std::endl;
  }
  auto k_t  = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
  auto k_t2 = FSys::last_write_time_point(source_path);

  REQUIRE(k_t == k_t2);

  auto k_s2 = source_path.remove_filename() / "测试文件2";
  {
    FSys::ofstream k_f{k_s2};
    k_f << "1";
  }
  FSys::last_write_time_point(k_s2, k_t2);
  REQUIRE(FSys::last_write_time_point(k_s2) == k_t2);
}

TEST_CASE_METHOD(test_path_, "test_path_3_folder_time", "[fun][path]") {
  struct _stat64 fileInfo {};
  root /= "tmp";
  if (_wstat64(root.generic_wstring().c_str(), &fileInfo) != 0) {
    std::cout << "Error : not find last_write_time " << std::endl;
  }
  auto k_t  = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
  auto k_t2 = FSys::last_write_time_point(root);
  REQUIRE(k_t == k_t2);
}
TEST_CASE_METHOD(test_path_, "test_path_3_file_sys_append", "[fun][path]") {
  auto k_path = source_path.replace_extension(".txt.backup");
  REQUIRE(k_path == FSys::path{"F:/测试文件.txt.backup"});
}

TEST_CASE_METHOD(test_path_, "test_path_3_path_relative", "[fun][path]") {
  auto k_p = p1.root_name() / p1.root_directory() / p1.relative_path();
  REQUIRE(k_p == FSys::path{"D:/9-houqi/Content/Character"});
  REQUIRE(p1.root_path() == FSys::path{"D:/"});
  REQUIRE(p1.root_directory() == FSys::path{"/"});
  REQUIRE(p1.lexically_relative(root) == FSys::path{"9-houqi/Content/Character"});
}

TEST_CASE_METHOD(test_path_, "test_path_3_path_open_ex", "[fun][path]") {
  FSys::open_explorer(source_path.parent_path());
  FSys::open_explorer(root);
}

class test_path_3 : public test_path_ {
 public:
  FSys::path dir_s{"D:/tmp"};
  FSys::path dir_d{"D:/tmp2"};
};

TEST_CASE_METHOD(test_path_3, "test_path_copy", "[fun][path]") {
  copy(dir_s, dir_d, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

TEST_CASE("core create_path", "[fun][create_path]") {
  using namespace doodle;
  auto k_prj = make_handle(g_reg()->create());
  k_prj.emplace<project>("D:/", "ttt");
  auto k_ass = make_handle(g_reg()->create());
  k_ass.emplace<assets>("assets");

  auto k_ass2 = make_handle(g_reg()->create());
  k_ass2.emplace<assets>("eee");

  auto k_ass_file = make_handle(g_reg()->create());
  k_ass_file.emplace<assets_file>("eee");
}

#include <opencv2/opencv.hpp>
TEST_CASE("core opencv", "[fun]") {
  using namespace doodle;
  const FSys::path path{R"(D:\tmp\image_test_ep002_sc001)"};
  std::fstream file{};
  int i      = 0;

  auto video = cv::VideoWriter("D:/test.mp4", cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), (double)25, cv::Size{1280, 720});
  cv::Mat image{};
  for (auto&& it_p : FSys::directory_iterator(path)) {
    if (FSys::is_regular_file(it_p)) {
      image = cv::imread(it_p.path().string());
      video << image;
    }
  }
}
TEST_CASE("core opencv image", "[fun]") {
  const FSys::path path{R"(D:\image_test\DB0106_sc0078_V.1001.png)"};
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

TEST_CASE("std basic_ios eof", "[std]") {
  std::istringstream s1{};
  std::istream l_iostream{s1.rdbuf()};
  std::cout << std::boolalpha << l_iostream.eof() << std::endl;
  l_iostream.get();
  std::cout << std::boolalpha << l_iostream.eof() << std::endl;
}
TEST_CASE("std future", "[std]") {
  std::future<std::string> p_out_str{};
  using namespace chrono::literals;
  std::cout << std::boolalpha;
  std::cout << p_out_str.valid() << std::endl;
  //  std::cout << magic_enum::enum_name(p_out_str.wait_for(1ns)) << std::endl;
}

TEST_CASE("maya get log", "[maya]") {
  core_set_init{}.read_file();

  scheduler_t k_loop{};
  auto k_mesg = make_handle();
  k_mesg.emplace<process_message>();
  boost::asio::post(make_process_adapter<maya_exe>(
      g_io_context().get_executor(),
      k_mesg,
      R"(C:\Users\TD\Source\Doodle\src\doodle_exe\main\test\test_maya_null.py)"
  ));

  //  std::int32_t i{0};
  while (!g_main_loop().empty()) {
    //    DOODLE_LOG_INFO("{}", i);
    g_main_loop().update({}, nullptr);
    //    ++i;
  }
}
TEST_CASE("maya time out", "[maya]") {
  core_set_init{}.read_file();
  core_set::getSet().timeout = 1;

  scheduler_t k_loop{};
  auto k_mesg = make_handle();
  k_mesg.emplace<process_message>();
  g_main_loop().attach<maya_exe>(
      k_mesg,
      R"(C:\Users\TD\Source\Doodle\src\doodle_exe\main\test\test_maya_null.py)"
  );
  g_main_loop().update({}, nullptr);
  g_main_loop().update({}, nullptr);
  k_mesg.get<process_message>().aborted();
  g_main_loop().update({}, nullptr);
  g_main_loop().update({}, nullptr);
  k_mesg.get<process_message>().aborted();
  g_main_loop().update({}, nullptr);
  g_main_loop().update({}, nullptr);
  k_mesg.get<process_message>().aborted();
  g_main_loop().update({}, nullptr);
}

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

TEST_CASE("boost rational", "[boost][rational]") {
  using rati = boost::rational<std::uint64_t>;
  auto k_i   = rati(1, 5);
  auto k_i2  = rati(1, 10);
  std::cout << boost::rational_cast<std::double_t>(((k_i + k_i2) / rati{5}) * rati{2}) << std::endl;
}

TEST_CASE("std regex", "[std][regex]") {
  std::cout.setf(std::ios_base::boolalpha);
  // 致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
  const static std::wregex fatal_error_znch{
      LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma 中保存)"};

  // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
  const static std::wregex fatal_error_en_us{
      LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma)"};

  std::cout << std::regex_search(L"致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存", fatal_error_znch)
            << std::endl;
  REQUIRE(std::regex_search(L"致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存", fatal_error_znch));
  std::cout << std::regex_search(L"Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma", fatal_error_en_us)
            << std::endl;
  REQUIRE(std::regex_search(L"Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma", fatal_error_en_us));
}

TEST_CASE("path iter", "[core]") {
  using namespace doodle;
  FSys::path k_path{R"(Users\teXiao\Documents\Unreal_Projects\test_tmp\test_tmp.uproject)"};
  for (auto& i : k_path) {
    std::cout << i << std::endl;
  }
  FSys::path k_path2{R"(\Users\teXiao\Documents\Unreal_Projects\test_tmp\test_tmp.uproject)"};
  for (auto& i : k_path2) {
    std::cout << i << std::endl;
  }
  FSys::path k_path3{R"(C:\Users\teXiao\Documents\Unreal_Projects\test_tmp\test_tmp.uproject)"};
  for (auto& i : k_path3) {
    std::cout << i << std::endl;
  }
}

#include <doodle_lib/core/image_loader.h>

TEST_CASE_METHOD(app, "load default image") {
  auto k_i  = image_loader{}.default_image();
  auto k_i2 = image_loader{}.error_image();
}

class test_time_warp : public app_command_base {
 public:
  chrono::sys_time_pos t1{chrono::sys_days{2020_y / 1 / 2} + 2h + 3min + 4s};
  chrono::local_time_pos t2{chrono::local_days{2020_y / 1 / 2} + 10h + 3min + 4s};
};

TEST_CASE_METHOD(test_time_warp, "test_time_warp") {
  time_point_wrap l_1{t1};
  time_point_wrap l_2{t2};
  time_point_wrap l_3{2020, 1, 2, 10, 3, 4};
  using namespace date;

  std::cout << "time_point_wrap l_1{t1}.get_local_time();                    " << (l_1.zoned_time_.get_local_time()) << "\n";
  std::cout << "time_point_wrap l_2{t2}.get_local_time();                    " << (l_2.zoned_time_.get_local_time()) << "\n";
  std::cout << "time_point_wrap l_3{2020, 1, 2, 10, 3, 4}.get_local_time();  " << (l_3.zoned_time_.get_local_time()) << "\n";
  std::cout << "time_point_wrap l_1{t1}.get_sys_time();                      " << (l_1.zoned_time_.get_sys_time()) << "\n";
  std::cout << "time_point_wrap l_2{t2}.get_sys_time();                      " << (l_2.zoned_time_.get_sys_time()) << "\n";
  std::cout << "time_point_wrap l_3{2020, 1, 2, 10, 3, 4}.get_sys_time();    " << (l_3.zoned_time_.get_sys_time()) << "\n";

  REQUIRE(l_1.zoned_time_ == l_2.zoned_time_);
  REQUIRE(l_1.zoned_time_ == l_3.zoned_time_);
  REQUIRE(l_1.zoned_time_ == l_3.zoned_time_);
}

#include <doodle_core/core/init_register.h>

namespace {
constexpr auto test_reg_l = []() {
  std::cout << ("初始化注册完成");
};
class test_init_impl : public init_register::registrar_lambda<test_reg_l, 1> {};
}  // namespace

class test_init_ : public app {
 public:
 protected:
};
TEST_CASE_METHOD(test_init_, "test_init_") {
  run();
  for (auto&& mat : entt::resolve())
    std::cout << fmt::format("{}\n", mat.info().name());
}

TEST_CASE("test_cross_frame_check mu1", "[core]") {
  doodle::gui::detail::cross_frame_check<std::int32_t> l_test{};

  l_test.connect([](const std::int32_t& in) {
    std::cout << "call int " << in << std::endl;
  });
  for (auto i = 0;
       i < 10;
       ++i) {
    //    l_test   = i;
    auto l_g = l_test();
    l_g      = (i == 1 || i == 2 || i == 3 || i == 4);
    if (l_g) {
      l_g ^ i;
      std::cout << i << std::endl;
    }
  }
}

TEST_CASE("test_cross_frame_check mu2", "[core]") {
  doodle::gui::detail::cross_frame_check<std::int32_t> l_test{};

  l_test.connect([](const std::int32_t& in) {
    std::cout << "call int " << in << std::endl;
  });
  for (auto i = 0;
       i < 10;
       ++i) {
    //    l_test   = i;
    auto l_g = l_test();
    l_g      = (i == 1 || i == 3 || i == 5);
    if (l_g) {
      l_g ^ i;
      std::cout << i << std::endl;
    }
  }
}

TEST_CASE("test_cross_frame_check mu3", "[core]") {
  doodle::gui::detail::cross_frame_check<std::int32_t> l_test{};

  l_test.connect([](const std::int32_t& in) {
    std::cout << "call int " << in << std::endl;
  });
  for (auto i = 0;
       i < 10;
       ++i) {
    auto l_g = l_test();
    l_g      = (i == 1 || i == 2 || i == 3);
    if (l_g) {
      l_g ^ i;
      std::cout << i << std::endl;
    }
  }
}

// #include <boost/algorithm/string.hpp>
// #include <boost/archive/iterators/base64_from_binary.hpp>
// #include <boost/archive/iterators/binary_from_base64.hpp>
// #include <boost/archive/iterators/transform_width.hpp>
//  std::string encode64(const std::string &val) {
//   using namespace boost::archive::iterators;
//   using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
//   auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
//   return tmp.append((3 - val.size() % 3) % 3, '=');
// }
//  std::string decode64(const std::string &val) {
//   using namespace boost::archive::iterators;
//   using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
//   return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
//     return c == '\0';
//   });
// }
