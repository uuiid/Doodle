//
// Created by td_main on 2023/9/6.
//

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
using namespace doodle;
int core_run_ue(int, char** const) {
  boost::asio::io_context l_io_context{};
  static FSys::path l_ue_path{R"(D:\Program Files\Epic Games\UE_5.2\Engine\Binaries\Win64\UnrealEditor-Cmd.exe)"};
  static std::string l_command_line{R"(--version)"};
  boost::asio::readable_pipe log_{l_io_context};
  if (log_.is_open()) {
    DOODLE_LOG_INFO("log_ is open");
  } else {
    DOODLE_LOG_INFO("log_ is not open");
  };
  std::string l_log_str{};
  log_.async_read_some(
      boost::asio::buffer(l_log_str),
      [&l_log_str](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
          DOODLE_LOG_ERROR("log_ error: {}", ec.message());
          return;
        }
        DOODLE_LOG_INFO("log_ bytes_transferred: {}", bytes_transferred);
        DOODLE_LOG_INFO("log_ l_log_str: {}", l_log_str);
      }
  );

  boost::asio::readable_pipe err_{l_io_context};
  std::string l_err_str{};
  err_.async_read_some(
      boost::asio::buffer(l_err_str),
      [&l_err_str](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
          DOODLE_LOG_ERROR("err_ error: {}", ec.message());
          return;
        }
        DOODLE_LOG_INFO("err_ bytes_transferred: {}", bytes_transferred);
        DOODLE_LOG_INFO("err_ l_err_str: {}", l_err_str);
      }
  );
  boost::process::v2::process l_child{
      l_io_context,
      l_ue_path.string(),
      {R"(-log)", R"(D:/test.uproject)"},
      boost::process::v2::process_stdio{log_, err_, {}}};
  l_io_context.run();
  l_child.wait();
  return 0;
}