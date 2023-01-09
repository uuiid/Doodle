#include "lib_fixtures.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process/args.hpp>
#include <boost/process/detail/child_decl.hpp>
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

using boost_test_sink_mt     = detail::boost_test_sink<std::mutex>;

lib_fixtures::lib_fixtures() = default;
lib_fixtures::~lib_fixtures() { doodle::logger_ctrl::get_log().refresh(); }

run_subprocess::run_subprocess(boost::asio::io_context& in_io)
    : io(in_io),
      child(),
      out_attr(),
      err_attr(),
      out_str(std::make_shared<boost::asio::streambuf>()),
      err_str(std::make_shared<boost::asio::streambuf>()) {}

void run_subprocess::run(const std::string& in_run_fun) {
  out_attr = std::make_shared<boost::process::async_pipe>(io);
  err_attr = std::make_shared<boost::process::async_pipe>(io);
  using namespace std::literals;
  child = std::make_shared<boost::process::child>(
      io, boost::process::exe = boost::dll::program_location(),
      boost::process::args = {"--log_level=message"s, "--color_output=true"s, fmt::format("--run_test={}", in_run_fun)},
      boost::process::std_out > *out_attr, boost::process::std_err > *err_attr
  );
  read_(out_attr, out_str);
  read_(err_attr, err_str);
}

void run_subprocess::read_(
    const std::shared_ptr<boost::process::async_pipe>& in_pipe, const std::shared_ptr<boost::asio::streambuf>& in_str
) {
  boost::asio::async_read_until(*in_pipe, *in_str, '\n', [in_str](boost::system::error_code in_code, std::size_t in_n) {
    if (!in_code) {
      std::string l_line{};
      std::istream l_istream{in_str.get()};
      std::getline(l_istream, l_line);
      BOOST_TEST_MESSAGE(l_line);
    } else {
      BOOST_TEST_ERROR(in_code.what());
    }
  });
}
