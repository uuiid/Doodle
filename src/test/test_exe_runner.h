#pragma once

#include <doodle_lib/exe_warp/exe_runner.h>

namespace doodle {

class test_exe_runner : public exe_runner {
 public:
  boost::asio::awaitable<void> run_maya(const maya_run_arg& in_arg, logger_ptr in_logger) override;
  boost::asio::awaitable<void> run_ue(const ue_run_arg& in_arg) override;
};

}  // namespace doodle
