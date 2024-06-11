#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/sqlite/kitsu_backend_sqlite.h>
#include <doodle_lib/launch/kitsu_supplement.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(xlsx_table)

constexpr static std::string_view g_token{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlwZSI6Im"
    "FjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjoxNzY0NjMzNjAw"
    "LCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684"
};

BOOST_AUTO_TEST_CASE(main) {
  const char* argv[] = {"test", "--config=D:/test_files/test_db/kitsu_supplement.json"};

  app_command<launch::kitsu_supplement_t> l_app_base{2, argv};
  // 创建基本的测试数据

  entt::handle l_handle{*g_reg(), g_reg()->create()};
  auto& l_user               = l_handle.emplace<user>();
  l_user.id_                 = boost::lexical_cast<boost::uuids::uuid>("69a8d093-dcab-4890-8f9d-c51ef065d03b");

  entt::entity l_user_entity = l_handle;
  using namespace std::chrono_literals;

  {  // 创建虚拟user, 用以检查调休等数据从钉钉中获取是否正常
    l_handle         = {*g_reg(), g_reg()->create()};
    auto& l_user     = l_handle.emplace<user>();
    l_user.id_       = boost::lexical_cast<boost::uuids::uuid>("ce6d3b4d-75aa-4e0f-90af-18b913df138a");
    l_user.mobile_   = "15827605754";

    // 2
    l_handle         = {*g_reg(), g_reg()->create()};
    auto& l_user_2   = l_handle.emplace<user>();
    l_user_2.id_     = boost::lexical_cast<boost::uuids::uuid>("5b5153a1-51a4-4376-9450-2c317e523cbe");
    l_user_2.mobile_ = "18056860368";

    // 3
    l_handle         = {*g_reg(), g_reg()->create()};
    auto& l_user_3   = l_handle.emplace<user>();
    l_user_3.id_     = boost::lexical_cast<boost::uuids::uuid>("78a1ec16-8a2d-4ae2-b0c9-7a8092694100");
    l_user_3.mobile_ = "15635681053";
  }

  l_handle = {*g_reg(), g_reg()->create()};
  l_handle.emplace<attendance_block>(attendance_block{
      .id_ = core_set::get_set().get_uuid(),
      .attendance_block_ =
          {
              attendance{
                  .id_ = core_set::get_set().get_uuid(),
                  .start_time_ =
                      chrono::zoned_time<chrono::microseconds>{
                          chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 9h
                      },
                  .end_time_ =
                      chrono::zoned_time<chrono::microseconds>{
                          chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 12h
                      },
                  .remark_ = "remark",
                  .type_   = attendance::att_enum::overtime,
              },
              attendance{
                  .id_ = core_set::get_set().get_uuid(),
                  .start_time_ =
                      chrono::zoned_time<chrono::microseconds>{
                          chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 18h
                      },
                  .end_time_ =
                      chrono::zoned_time<chrono::microseconds>{
                          chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 22h
                      },
                  .remark_ = "remark",
                  .type_   = attendance::att_enum::leave,
              },
          },
      .create_date_ = chrono::year_month_day{2024y / 5 / 8},
      .update_time_ =
          chrono::zoned_time<chrono::microseconds>{
              chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
          },
      .user_ref_id_ = l_user_entity,
  });

  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_SUITE_END()