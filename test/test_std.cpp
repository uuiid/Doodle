#include <DoodleLib/DoodleLib.h>
#include <Windows.h>
#include <cryptopp/base64.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <date/date.h>
#include <date/tz.h>
#include <gtest/gtest.h>

#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

// #include <boost/nowide/
//

TEST(DSTD, dir) {
  auto r = 2740700278 / 8000000;
  std::cout << r << std::endl;
}
TEST(DSTD, num) {
#define DOODLE_tset_num(exp) #exp ": " << std::to_string(exp)
  std::cout << DOODLE_tset_num(10 % 2) << std::endl
            << DOODLE_tset_num(11 % 2) << std::endl
            << DOODLE_tset_num(10 / 2) << std::endl
            << DOODLE_tset_num(11 / 2) << std::endl;

#undef DOODLE_tset_num
}

TEST(DSTD, stdStringLocale) {
}

TEST(DSTD, std_locale) {
  auto data = boost::locale::conv::to_utf<wchar_t>("中", "UTF-8");
  std::cout << data.size() << std::endl;
  std::wcout << data.at(0) << std::endl;
  std::wcout << data.data() << std::endl;
  const std::string data2{};
  // data2
}

TEST(DSTD, regex) {
  std::regex regex{"."};
}

TEST(DSTD, u8stringAndString) {
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  //  std::locale::global(std::locale{".UTF8"});
  std::setlocale(LC_CTYPE, ".UTF8");
  std::filesystem::path str{L"D:/哈哈/scoo+1235"};
  boost::filesystem::path str_b{"D:/哈哈/scoo+1235"};
  // std::cout << std::string{str2.begin(), str2.end()} << std::endl;
  std::cout << "str: " << str << std::endl;

  std::cout << "generic_u8string: " << str.generic_u8string() << std::endl;
  std::cout << "generic_string: " << str.generic_string() << std::endl;
  std::cout << "boost conv: " << boost::locale::conv::utf_to_utf<char>(str.generic_wstring()) << std::endl;
  std::cout << "boost path: " << str_b << std::endl;
  std::cout << "boost path: " << str_b.generic_string() << std::endl;

  std::filesystem::create_directories(str);

  std::cout << "std::locale : " << std::locale{}.name().c_str() << std::endl;
  std::cout << "boost::locale : " << std::use_facet<boost::locale::info>(boost::locale::generator().generate("")).name() << std::endl;

  std::cout << typeid(std::filesystem::path).name() << std::endl;
  std::cout << typeid(std::cout).name() << std::endl;
}

TEST(DSTD, file_last_time) {
  auto file  = std::filesystem::path{u8"D:/test2.mp4"};
  auto file2 = std::filesystem::path{u8"D:/tmp"};
  auto time  = std::filesystem::last_write_time(file);

  auto time2 = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - decltype(time)::clock::now() + std::chrono::system_clock::now());
  auto time3 = std::chrono::system_clock::to_time_t(time2);
  std::string str{};
  str.resize(100);
  tm k_tm{};
  localtime_s(&k_tm, &time3);
  asctime_s(str.data(), 100, &k_tm);
  std::cout << str << std::endl;
}

TEST(DSTD, regerFind_eps_shot) {
  std::regex k_exp_epis{R"(ep_?(\d+))", std::regex_constants::icase};
  std::regex k_exp_shot{R"(sc_?(\d+)([a-z])?)", std::regex_constants::icase};
  std::string str{R"(D:\Sc_064B\sc_064a\sc064\sc_064_\BuJu.1001.png)"};
  std::string str_ep{R"(D:\Ep_064B\ep_064a\ep064\ep_064_\BuJu.1001.png)"};
  std::smatch k_match{};
  while (std::regex_search(str, k_match, k_exp_shot)) {
    std::cout << "\n匹配:" << std::endl;
    for (auto i = 0; i < k_match.size(); ++i)
      std::cout << k_match[i].str() << std::endl;

    str = k_match.suffix();
  }
  while (std::regex_search(str_ep, k_match, k_exp_epis)) {
    std::cout << "\n匹配:" << std::endl;
    for (auto i = 0; i < k_match.size(); ++i)
      std::cout << k_match[i].str() << std::endl;

    str_ep = k_match.suffix();
  }
}

TEST(DSTD, date_check) {
  using namespace date;

  date::sys_days t{2010_y / 2 / 30};
  std::cout << t << std::endl;

  doodle::TimeDuration my_t{};
  my_t.set_year(2012);
  my_t.set_month(2);
  my_t.set_day(60);
  std::cout << my_t.getUTCTime() << std::endl;
  std::cout << my_t.get_day() << std::endl;
}

TEST(DSTD, date_utc) {
  using namespace date;
  using namespace std::chrono_literals;
  auto my_t       = std::chrono::system_clock::now();
  auto my_local_t = make_zoned(current_zone(), std::chrono::system_clock::now());

  auto mt_tt = local_days{2021_y / 06 / 16} + 10h + 34min + 37s;

  auto k_dp = date::floor<date::days>(my_local_t.get_local_time());
  date::year_month_day k_day{k_dp};
  date::hh_mm_ss k_hh_mm_ss{date::floor<std::chrono::milliseconds>(my_local_t.get_local_time() - k_dp)};

  std::cout << my_t << "\n"
            << my_local_t << "\n"
            << "my_local_t.get_sys_time(): " << my_local_t.get_sys_time() << "\n"
            << "my_local_t.get_local_time(): " << my_local_t.get_local_time() << "\n"
            << R"(date::format("%Y/%m/%d %H:%M"): )" << date::format("%Y/%m/%d %H:%M", my_local_t) << "\n"
            << "my_t to utc: " << to_utc_time(my_t) << "\n"
            << "my_local_t to utc " << to_utc_time(my_local_t.get_sys_time()) << "\n"
            << "make_zoned(current_zone(), mt_tt).get_sys_time(): " << make_zoned(current_zone(), mt_tt).get_sys_time() << "\n"
            << "make_zoned(current_zone(), mt_tt).get_local_time(): " << make_zoned(current_zone(), mt_tt).get_local_time() << "\n"

            << "\n"
            << "std::chrono::system_clock::now(): " << std::chrono::system_clock::now() << "\n"
            << "date::clock_cast<date::local_t>(std::chrono::system_clock::now()): " << date::clock_cast<date::local_t>(std::chrono::system_clock::now()) << "\n"
            << "date::make_zoned(date::current_zone(),std::chrono::system_clock::now()): " << date::make_zoned(date::current_zone(), std::chrono::system_clock::now()) << "\n"
            << "注意这里是不对的-> date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())): " << date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())) << "\n"
            << "注意这里是不对的-> date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())).get_local_time(): " << date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())).get_local_time() << "\n"
            << "注意这里是不对的-> date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())).get_sys_time(): " << date::make_zoned(date::current_zone(), date::clock_cast<date::local_t>(std::chrono::system_clock::now())).get_sys_time() << "\n"
            << "date::clock_cast<date::utc_clock>(std::chrono::system_clock::now()): " << date::clock_cast<date::utc_clock>(std::chrono::system_clock::now()) << "\n"
            << "\n"

            << "\n"
            << "year   : " << k_day.year() << "\n"
            << "month  : " << k_day.month() << "\n"
            << "day    : " << k_day.day() << "\n"
            << "hours  : " << k_hh_mm_ss.hours() << "\n"
            << "minutes: " << k_hh_mm_ss.minutes() << "\n"
            << "seconds: " << k_hh_mm_ss.seconds() << "\n"

            << std::endl;
}

TEST(DSTD, time_duration){

}

TEST(DSTD, observable_container) {
  using namespace doodle;
  //  using my_str = observable_container<std::vector<std::string>, details::pre<std::vector<std::string> > >;
  //  details::pre<std::vector<std::string> > k_pre{};
  //  my_str test{&k_pre};

  using my_str = observable_container<std::vector<std::string> >;
  my_str test{};

  test.sig_clear.connect([]() { std::cout << "clear" << std::endl; });
  test.sig_insert.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_erase.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_push_back.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_resize.connect([](std::size_t where) { std::cout << where << std::endl; });
  test.sig_swap.connect([](const std::vector<std::string>& strs) {
    for (auto& s : strs) {
      std::cout << s << ",";
    }
    std::cout << std::endl;
  });
  test.push_back_sig({"test"});
  test.push_back_sig({"tesat"});
  test.push_back_sig({"22222"});
  test.insert_sig(test.begin(), {"11111"});
  test.erase_sig(test.begin());
  test.resize_sig(2);
  auto k_other = std::vector<std::string>{"1", "2", "3", "4"};
  test.swap_sig(k_other);
  test.clear_sig();
}
class TT {};

TEST(DSTD, find) {
  using namespace doodle;

  using my_tt = observable_container<std::vector<std::shared_ptr<TT> > >;
  auto ta1    = std::make_shared<TT>();
  auto ta2    = std::make_shared<TT>();
  auto ta3    = std::make_shared<TT>();

  my_tt test{};
  test.push_back(ta2);
  test.push_back(ta3);
  test.push_back(ta1);

  test.push_back(std::shared_ptr<TT>{ta1});

  test.erase_sig(std::shared_ptr<TT>{ta1});
}

TEST(DSTD, filesystem_add_time_stamp) {
  std::cout << doodle::FSys::add_time_stamp("/test/dasd") << "\n"
            << doodle::FSys::add_time_stamp("/test/dasd.mp4")
            << std::endl;
}

TEST(DSTD, filesystem_open_ex) {
  using namespace doodle;
  FSys::open_explorer(LR"(D:\哈哈\scoo+1235)");
  FSys::open_explorer(LR"(D:\OneDrie)");
  FSys::open_explorer(LR"(D:\OneDrie\VFXFORCE\快速提示01-Vimeo上Houdini中的生长繁殖.mp4)");
}

TEST(DSTD, hash_file) {
  std::cout << doodle::FSys::file_hash_sha224(R"(D:\Houdini 18.5.462 Win.rar)") << std::endl;
}
