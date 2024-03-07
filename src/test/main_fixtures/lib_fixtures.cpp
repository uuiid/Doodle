#include "lib_fixtures.h"

#include <doodle_core/platform/win/get_prot.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process/args.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test_log.hpp>

#include <fmt/core.h>
#include <memory>
#include <vector>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <spdlog/sinks/base_sink.h>

namespace detail {
template <class Mutex>
class boost_test_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  boost_test_sink() = default;

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    BOOST_TEST_MESSAGE(fmt::to_string(formatted));
  }

  void flush_() override {}
};
}  // namespace detail

using boost_test_sink_mt      = detail::boost_test_sink<std::mutex>;

lib_fixtures::lib_fixtures()  = default;
lib_fixtures::~lib_fixtures() = default;
