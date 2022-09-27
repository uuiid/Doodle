#include "lib_fixtures.h"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <spdlog/sinks/base_sink.h>

namespace detail {
template <class Mutex>
class boost_test_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  boost_test_sink() = default;

 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    BOOST_TEST_MESSAGE(fmt::to_string(formatted));
  }

  void flush_() override {}
};
}  // namespace detail

using boost_test_sink_mt = detail::boost_test_sink<std::mutex>;

lib_fixtures::lib_fixtures() {
  doodle_lib_attr.create_time_database();
  doodle::logger_ctrl::get_log().add_log_sink(
      std::make_shared<boost_test_sink_mt>()
  );
}