#include "test_exe_runner.h"

#include <doodle_core/lib_warp/maya_exe_out.h>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <fstream>

namespace doodle {

boost::asio::awaitable<void> test_exe_runner::run_maya(const maya_run_arg& in_arg, logger_ptr in_logger) {
  in_logger->warn("测试环境 stub 运行 maya {}", in_arg.key_);
  if (!FSys::exists(in_arg.arg_file_.parent_path())) FSys::create_directories(in_arg.arg_file_.parent_path());
  nlohmann::json l_json = maya_out_arg{};
  std::ofstream l_file{in_arg.arg_file_};
  l_file << l_json.dump();
  co_return;
}

boost::asio::awaitable<void> test_exe_runner::run_ue(const ue_run_arg& in_arg) {
  in_arg.logger_->warn("测试环境 stub 运行 ue {}", in_arg.in_arg_);
  co_return;
}

}  // namespace doodle
