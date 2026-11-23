//
// Created by TD on 25-11-23.
//

#pragma once

#include <doodle_core/lib_warp/maya_exe_out.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace doodle {

namespace kitsu {
class kitsu_client;
}

struct maya_run_arg {
  std::string key_{};
  std::string json_{};
  FSys::path mel_file_{};
  FSys::path arg_file_{};
};

struct ue_run_arg {
  std::vector<std::string> in_arg_{};
  logger_ptr logger_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};
  std::shared_ptr<server_task_info::run_time_info_t> time_info_{};
};

struct exe_run_process_arg {
  FSys::path program_{};
  std::vector<std::string> args_{};
  FSys::path work_dir_{};
  std::unordered_map<std::wstring, std::wstring> env_{};
  std::string out_code_{"GBK"};
};

class DOODLELIB_API exe_runner {
 public:
  exe_runner()          = default;
  virtual ~exe_runner() = default;

  virtual boost::asio::awaitable<void> run_maya(const maya_run_arg& in_arg, logger_ptr in_logger) = 0;
  virtual boost::asio::awaitable<void> run_ue(const ue_run_arg& in_arg)                           = 0;
};

class DOODLELIB_API exe_runner_prod : public exe_runner {
 protected:
  virtual boost::asio::awaitable<maya_out_arg> run_process(const exe_run_process_arg& in_arg, logger_ptr in_logger) = 0;

 public:
  boost::asio::awaitable<void> run_maya(const maya_run_arg& in_arg, logger_ptr in_logger) override;
};

class DOODLELIB_API exe_runner_process : public exe_runner_prod {
 protected:
  boost::asio::awaitable<maya_out_arg> run_process(const exe_run_process_arg& in_arg, logger_ptr in_logger) override;

 public:
  boost::asio::awaitable<void> run_ue(const ue_run_arg& in_arg) override;
};

}  // namespace doodle
