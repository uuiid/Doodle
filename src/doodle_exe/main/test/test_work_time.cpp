//
// Created by TD on 2022/6/14.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/time_tool/work_clock.h>

#include <catch.hpp>
#include <catch2/catch_approx.hpp>

using namespace doodle;

class test_time_duration : public app {
 public:
  chrono::sys_seconds p_new     = chrono::sys_days{2021_y / 06 / 16} + 10h + 34min + 37s;
  chrono::local_seconds p_local = chrono::local_days{2021_y / 06 / 16} + 18h + 34min + 37s;

  time_point_wrap time_1_a{chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s};  /// \brief 周三
  time_point_wrap time_1_b{chrono::local_days(2021_y / 7 / 23_d) + 16h + 20min + 30s};  /// \brief 周五

  time_point_wrap time_2_a{chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s};
  time_point_wrap time_2_b{chrono::local_days(2021_y / 7 / 27_d) + 16h + 20min + 30s};

  time_point_wrap time_3_a{chrono::local_days(2021_y / 6 / 23_d) + 17h + 8min + 48s};
  time_point_wrap time_3_b{chrono::local_days(2021_y / 6 / 23_d) + 20h + 8min + 48s};

  time_point_wrap time_4_a{chrono::sys_days(2021_y / 7 / 21_d) + 2h + 45min + 30s};
  time_point_wrap time_4_b{chrono::sys_days(2021_y / 7 / 23_d) + 8h + 20min + 30s};

  time_point_wrap time_5_a{chrono::sys_days(2021_y / 7 / 21_d) + 2h + 45min + 30s};
  time_point_wrap time_5_b{chrono::sys_days(2021_y / 7 / 27_d) + 8h + 20min + 30s};

  time_point_wrap time_6_a{chrono::sys_days(2021_y / 6 / 23_d) + 9h + 8min + 48s};
  time_point_wrap time_6_b{chrono::sys_days(2021_y / 6 / 23_d) + 12h + 8min + 48s};

  time_point_wrap time_7_a{chrono::local_days(2022_y / 2 / 22_d) + 3h + 41min + 29s};   /// \brief 周二
  time_point_wrap time_7_b{chrono::local_days(2022_y / 2 / 28_d) + 10h + 41min + 28s};  /// \brief 周一

  time_point_wrap time_8_a{chrono::local_days(2022_y / 2 / 22_d) + 3h + 41min + 29s};   /// \brief 周二
  time_point_wrap time_8_b{chrono::local_days(2022_y / 2 / 28_d) + 10h + 41min + 28s};  /// \brief 周一
  //
  //  time_point_wrap time_8_a;
  //  time_point_wrap time_8_b;
};

TEST_CASE_METHOD(test_time_duration, "test_time_duration1") {
  auto k_time = chrono::make_zoned(chrono::current_zone(), p_local);
  REQUIRE(k_time.get_sys_time() == p_new);
}

TEST_CASE_METHOD(test_time_duration, "test_time_duration2") {
  auto k_time = chrono::make_zoned(chrono::current_zone(), p_new);
  REQUIRE(k_time.get_local_time() == p_local);
}
TEST_CASE_METHOD(test_time_duration, "work_time") {
  using namespace Catch::literals;

#define DOODLE_T_M_1(time_index, time_du, rules_)                                                                                                                                                 \
  REQUIRE(doodle::work_duration(time_##time_index##_a.zoned_time_.get_local_time(), time_##time_index##_b.zoned_time_.get_local_time(), rules_).count() == Catch::Approx{time_du}.epsilon(0.01)); \
  std::cout << "\n"                                                                                                                                                                               \
            << "work duration  "                                                                                                                                                                  \
            << doodle::work_duration(time_##time_index##_a.zoned_time_.get_local_time(), time_##time_index##_b.zoned_time_.get_local_time(), rules_).count()                                      \
            << " = " << #time_du                                                                                                                                                                  \
            << "\n"                                                                                                                                                                               \
            << time_##time_index##_a.zoned_time_.get_local_time() << "\n"                                                                                                                         \
            << "next " << doodle::chrono::hours_double{time_du}.count() << "/h \n"                                                                                                                \
            << doodle::next_time(time_##time_index##_a.zoned_time_.get_local_time(), doodle::chrono::hours_double{time_du}, rules_) << "\n"                                                       \
            << time_##time_index##_b.zoned_time_.get_local_time()                                                                                                                                 \
            << std::endl;

  doodle::business::rules l_tset_rules_def{};
  doodle::business::rules l_tset_rules{};
  l_tset_rules.extra_rest.emplace_back(
      std::make_pair(chrono::local_days(2022_y / 2 / 24_d) + 8h, chrono::local_days(2022_y / 2 / 24_d) + 18h)
  );

  DOODLE_T_M_1(1, 20.5817, l_tset_rules_def);
  DOODLE_T_M_1(2, 36.5806, l_tset_rules_def);
  DOODLE_T_M_1(3, 0.853056, l_tset_rules_def);
  DOODLE_T_M_1(4, 20.5817, l_tset_rules_def);
  DOODLE_T_M_1(5, 36.5806, l_tset_rules_def);
  DOODLE_T_M_1(6, 0.853056, l_tset_rules_def);
  DOODLE_T_M_1(7, 33.6886, l_tset_rules_def);
  DOODLE_T_M_1(8, 25.6892, l_tset_rules);

  l_tset_rules.extra_rest.emplace_back(
      std::make_pair(chrono::local_days(2022_y / 2 / 25_d) + 8h, chrono::local_days(2022_y / 2 / 25_d) + 18h)
  );
  DOODLE_T_M_1(8, 17.6897, l_tset_rules);

  l_tset_rules.extra_work.emplace_back(
      std::make_pair(chrono::local_days(2022_y / 2 / 26_d) + 8h, chrono::local_days(2022_y / 2 / 26_d) + 18h)
  );
  DOODLE_T_M_1(8, 27.6894, l_tset_rules);

  l_tset_rules.extra_work.clear();

  l_tset_rules.extra_work.emplace_back(
      std::make_pair(chrono::local_days(2022_y / 2 / 26_d) + 8h, chrono::local_days(2022_y / 2 / 26_d) + 12h)
  );
  l_tset_rules.extra_work.emplace_back(
      std::make_pair(chrono::local_days(2022_y / 2 / 26_d) + 13h, chrono::local_days(2022_y / 2 / 26_d) + 18h)
  );
  DOODLE_T_M_1(8, 26.6892, l_tset_rules);
#undef DOODLE_T_M_1
}
