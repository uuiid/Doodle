//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/metadata/image_size.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>

#include <argh.h>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <utility>

namespace doodle {
class maya_exe;

namespace maya_exe_ns {

struct maya_out_arg {
  std::uint32_t begin_time{};
  std::uint32_t end_time{};
  std::vector<FSys::path> out_file_list{};
  FSys::path movie_file_dir{};

  friend void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg& nlohmann_json_t);

  friend void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg& nlohmann_json_t);
};

class arg : public async_task {
  FSys::path out_path_file_{};
  std::shared_ptr<server_task_info::run_time_info_t> time_info_;

 protected:
  FSys::path file_path{};
  maya_out_arg out_arg_{};

 public:
  arg()          = default;
  virtual ~arg() = default;

  // get set attr
  FSys::path get_out_path_file() const { return out_path_file_; }
  void set_time_info(std::shared_ptr<server_task_info::run_time_info_t> in_time_info) { time_info_ = in_time_info; }
  std::shared_ptr<server_task_info::run_time_info_t> get_time_info() const { return time_info_; }
  void set_file_path(FSys::path in_path) { file_path = in_path; }
  FSys::path get_file_path() const { return file_path; }

  // get out arg
  maya_out_arg get_out_arg() const { return out_arg_; }

  // form json
  friend void from_json(const nlohmann::json& in_json, arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const arg& out_obj);
  virtual std::tuple<std::string, std::string> get_json_str() = 0;

  boost::asio::awaitable<void> async_run_maya();
};




class DOODLELIB_API replace_file_arg : public maya_exe_ns::arg {
 public:
  std::vector<std::pair<FSys::path, FSys::path>> file_list{};
  constexpr static std::string_view k_name{"replace_file"};

  // form json
  friend void from_json(const nlohmann::json& in_json, replace_file_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const replace_file_arg& out_obj);
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  boost::asio::awaitable<void> run() override;
};

/**
 * @brief 导出骨骼
 * 使用 run 导出时, 会自动将 maya 文件向上一级移动, 并备份文件
 * 使用 async_run_maya 导出时, 不会移动文件
 */
class DOODLELIB_API export_rig_arg : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"export_rig"};
  // to json
  friend void to_json(nlohmann::json& in_json, const export_rig_arg& out_obj);
  boost::asio::awaitable<void> run() override;

  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
};

FSys::path find_maya_path();
}  // namespace maya_exe_ns

class maya_ctx {
 public:
  maya_ctx()  = default;
  ~maya_ctx() = default;
  std::shared_ptr<awaitable_queue_limitation> queue_ =
      std::make_shared<awaitable_queue_limitation>(core_set::get_set().p_max_thread);
};

}  // namespace doodle