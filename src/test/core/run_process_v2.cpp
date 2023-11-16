//
// Created by TD on 2023/11/16.
//

// #include <boost/process/v2.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(run_process_v2) {
  //  boost::process::v2::process_environment l_env{
  //      std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value>{
  //          {"MAYA_LOCATION", ""}, {"PATH", ""}}};
  //  boost::asio::readable_pipe l_out_pipe{g_io_context()};
  //  boost::asio::readable_pipe l_err_pipe{g_io_context()};
  //  boost::process::v2::process_stdio l_io{nullptr, l_out_pipe, l_err_pipe};
  //  boost::process::v2::process l_process{
  //      g_io_context(), program_path, {fmt::format("--{}={}", run_script_attr_key, l_path)}, l_env, l_io};
  //  boost::asio::cancellation_signal sig{};
  //  boost::process::v2::async_execute(
  //      boost::process::v2::process{
  //          g_io_context(), program_path, {fmt::format("--{}={}", run_script_attr_key, l_path)}, l_env, l_io},
  //      boost::asio::bind_cancellation_slot(sig, [](boost::system::error_code ec, int exit_code) {})
  //  );
}