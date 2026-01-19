#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu/computing_time.h>
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

BOOST_AUTO_TEST_CASE(main) {}

BOOST_AUTO_TEST_SUITE_END()