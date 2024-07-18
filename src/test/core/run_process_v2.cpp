//
// Created by TD on 2023/11/16.
//

#include <boost/process/v2.hpp>
#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <fmt/format.h>
BOOST_AUTO_TEST_CASE(run_process_v2) {
  const static std::string program_path{"cmd.exe"};
  boost::asio::io_context l_io_context{};
  return;
  boost::process::v2::process_environment l_env{
      std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value>{
          {"MAYA_LOCATION", ""}, {"PATH", ""}}};
  boost::asio::readable_pipe l_out_pipe{l_io_context};
  boost::asio::readable_pipe l_err_pipe{l_io_context};
  boost::process::v2::process_stdio l_io{nullptr, l_out_pipe, l_err_pipe};
  boost::process::v2::process l_process{
      l_io_context, program_path, {fmt::format("--{}={}", "", "")}, l_env, l_io};
  boost::asio::cancellation_signal sig{};
  boost::process::v2::async_execute(
    boost::process::v2::process{
        l_io_context, program_path, {fmt::format("--{}={}", "", "")}, l_env, l_io},
    [](boost::system::error_code ec, int exit_code) {
    }
  );
}