#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>

#include <doodle_core/core/app_base.h>


#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/launch/kitsu_supplement.h>

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(xlsx_table)

BOOST_AUTO_TEST_CASE(msg_body) {
  boost::beast::http::response<doodle::http::basic_json_body> l_respone{boost::beast::http::status::ok, 11};
  l_respone.body() = nlohmann::json{{"msg", "hello world"}};
  l_respone.prepare_payload();
  // boost::beast::http::message_generator l_gen{std::move(l_respone)};
  std::ostringstream l_str{};
  l_str << l_respone;
  default_logger_raw()->info(l_str.str());
}

BOOST_AUTO_TEST_CASE(file_s) {
  boost::iostreams::filtering_stream<boost::iostreams::input> stream_{};
  stream_.push(boost::iostreams::file_source{"E:/1.txt"});
  char l_str[2048]{};
  BOOST_ASSERT(stream_);
  stream_.sync();
  stream_.read(l_str, 2048);
  std::cout << "读取文件: \n" << l_str << std::endl;
  // std::cout << "读取文件asdsa2: \n" << stream_.rdbuf() << std::endl;
}

constexpr static std::string_view g_token{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlwZSI6Im"
    "FjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjoxNzY0NjMzNjAw"
    "LCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684"
};

BOOST_AUTO_TEST_CASE(main) {
  const char* argv[] = {"test", "--config=E:/doodle/build/debug_server.json"};

  app_command<launch::kitsu_supplement_t> l_app_base{2, argv};
  // 创建基本的测试数据

  entt::handle l_handle{*g_reg(), g_reg()->create()};
  auto& l_user               = l_handle.emplace<user>();
  l_user.id_                 = boost::lexical_cast<boost::uuids::uuid>("69a8d093-dcab-4890-8f9d-c51ef065d03b");
  // l_user.dingding_company_id_ = boost::lexical_cast<boost::uuids::uuid>("fd3eb038-7cd5-46bf-88f6-c8e6097d9325");

  entt::entity l_user_entity = l_handle;
  using namespace std::chrono_literals;

  {  // 创建虚拟user, 用以检查调休等数据从钉钉中获取是否正常
    l_handle              = {*g_reg(), g_reg()->create()};
    auto& l_user          = l_handle.emplace<user>();
    l_user.id_            = boost::lexical_cast<boost::uuids::uuid>("ce6d3b4d-75aa-4e0f-90af-18b913df138a");
    l_user.mobile_        = "15827605754";
    // l_user.dingding_company_id_   = boost::lexical_cast<boost::uuids::uuid>("fd3eb038-7cd5-46bf-88f6-c8e6097d9325");
    l_user.dingding_id_   = "16951873382881136";

    // 2
    l_handle              = {*g_reg(), g_reg()->create()};
    auto& l_user_2        = l_handle.emplace<user>();
    l_user_2.id_          = boost::lexical_cast<boost::uuids::uuid>("5b5153a1-51a4-4376-9450-2c317e523cbe");
    l_user_2.mobile_      = "18056860368";
    // l_user_2.dingding_company_id_ = boost::lexical_cast<boost::uuids::uuid>("fd3eb038-7cd5-46bf-88f6-c8e6097d9325");
    l_user_2.dingding_id_ = "16941375683116574";

    // 3
    l_handle              = {*g_reg(), g_reg()->create()};
    auto& l_user_3        = l_handle.emplace<user>();
    l_user_3.id_          = boost::lexical_cast<boost::uuids::uuid>("78a1ec16-8a2d-4ae2-b0c9-7a8092694100");
    l_user_3.mobile_      = "15635681053";
    // l_user_3.dingding_company_id_ = boost::lexical_cast<boost::uuids::uuid>("fd3eb038-7cd5-46bf-88f6-c8e6097d9325");
    l_user_3.dingding_id_ = "250801386824116912";
  }

  l_handle = {*g_reg(), g_reg()->create()};
  // l_handle.emplace<attendance_block>(attendance_block{
  //     .id_ = core_set::get_set().get_uuid(),
  //     .attendance_block_ =
  //         {
  //             attendance{
  //                 .id_ = core_set::get_set().get_uuid(),
  //                 .start_time_ =
  //                     chrono::zoned_time<chrono::microseconds>{
  //                         chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 9h
  //                     },
  //                 .end_time_ =
  //                     chrono::zoned_time<chrono::microseconds>{
  //                         chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 12h
  //                     },
  //                 .remark_ = "remark",
  //                 .type_   = attendance::att_enum::overtime,
  //             },
  //             attendance{
  //                 .id_ = core_set::get_set().get_uuid(),
  //                 .start_time_ =
  //                     chrono::zoned_time<chrono::microseconds>{
  //                         chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 18h
  //                     },
  //                 .end_time_ =
  //                     chrono::zoned_time<chrono::microseconds>{
  //                         chrono::current_zone(), chrono::local_days{2024y / 5 / 8} + 22h
  //                     },
  //                 .remark_ = "remark",
  //                 .type_   = attendance::att_enum::leave,
  //             },
  //         },
  //     .create_date_ = chrono::year_month_day{2024y / 5 / 8},
  //     .update_time_ =
  //         chrono::zoned_time<chrono::microseconds>{
  //             chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
  //         },
  //     .user_ref_id_ = l_user_entity,
  // });
  // l_user.attendance_block_[chrono::year_month_day{2024y / 5 / 8}] = l_handle;
  //
  // try {
  //   // 工作守卫
  //   auto l_work_guard = boost::asio::make_work_guard(g_io_context());
  //   g_io_context().run();
  // } catch (const std::exception& e) {
  //   BOOST_TEST_MESSAGE(e.what());
  // }
}

BOOST_AUTO_TEST_SUITE_END()