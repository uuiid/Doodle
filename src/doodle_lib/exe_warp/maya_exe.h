//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::maya_exe_ns {

class arg {
 public:
  arg()          = default;
  virtual ~arg() = default;
  FSys::path file_path;
  FSys::path project_;
  std::int32_t t_post;
  std::int32_t export_anim_time;
  friend void to_json(nlohmann::json &nlohmann_json_j, const arg &nlohmann_json_t) {
    nlohmann_json_j["path"]             = nlohmann_json_t.file_path.generic_string();
    nlohmann_json_j["project_"]         = nlohmann_json_t.project_.generic_string();
    nlohmann_json_j["t_post"]           = nlohmann_json_t.t_post;
    nlohmann_json_j["export_anim_time"] = nlohmann_json_t.export_anim_time;
  }
};

class DOODLELIB_API qcloth_arg : public maya_exe_ns::arg {
 public:
  bool only_sim;
  bool upload_file;
  bool export_fbx;
  bool only_export;

  friend void to_json(nlohmann::json &nlohmann_json_j, const qcloth_arg &nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg &>(nlohmann_json_t));
    nlohmann_json_j["only_sim"]    = nlohmann_json_t.only_sim;
    nlohmann_json_j["upload_file"] = nlohmann_json_t.upload_file;
    nlohmann_json_j["export_fbx"]  = nlohmann_json_t.export_fbx;
    nlohmann_json_j["only_export"] = nlohmann_json_t.only_export;
  };
};

class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool use_all_ref;
  bool upload_file;

  friend void to_json(nlohmann::json &nlohmann_json_j, const export_fbx_arg &nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg &>(nlohmann_json_t));
    nlohmann_json_j["use_all_ref"] = nlohmann_json_t.use_all_ref;
    nlohmann_json_j["upload_file"] = nlohmann_json_t.upload_file;
  };
};

class DOODLELIB_API replace_file_arg : public maya_exe_ns::arg {
 public:
  bool replace_file_all;

  friend void to_json(nlohmann::json &nlohmann_json_j, const replace_file_arg &nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg &>(nlohmann_json_t));
    nlohmann_json_j["replace_file_all"] = nlohmann_json_t.replace_file_all;
  };
};
}  // namespace doodle::maya_exe_ns
namespace doodle {
class DOODLELIB_API maya_exe : public process_t<maya_exe> {
  class impl;
  std::unique_ptr<impl> p_i;
  static void add_maya_fun_tool();
  template <typename T>
  explicit maya_exe(const entt::handle &in_handle, const T &in_arg, std::int32_t in_arg_tag);

 public:
  using base_type = process_t<maya_exe>;

  //  void succeed() noexcept;
  //  void fail() noexcept;
  //  void pause() noexcept;
  /**
   * @brief 运行mayapy 任意python脚本
   * @param in_handle 具有消息组件的的句柄
   * @param in_file py文件路径
   *
   * 检查 process_message 和 core_set::get_set().has_maya()
   *
   */
  explicit maya_exe(const entt::handle &in_handle, const std::string &in_file);
  /**
   * @brief 使用配置进行qcloth操作
   * @param in_handle 具有消息组件的的句柄
   * @param in_arg qcloth 配置类
   *
   * 检查 process_message 和 core_set::get_set().has_maya()
   *
   */
  explicit maya_exe(const entt::handle &in_handle, const maya_exe_ns::qcloth_arg &in_arg);
  /**
   * @brief 使用配置进行fbx导出
   * @param in_handle 具有消息组件的的句柄
   * @param in_arg 导出fbx 配置类
   *
   * 检查 process_message 和 core_set::get_set().has_maya()
   *
   */
  explicit maya_exe(const entt::handle &in_handle, const maya_exe_ns::export_fbx_arg &in_arg);
  /**
   * @brief 使用配置进行文件替换
   * @param in_handle 具有消息组件的的句柄
   * @param in_arg 导出 文件替换 配置类
   *
   * 检查 process_message 和 core_set::get_set().has_maya()
   *
   */
  explicit maya_exe(const entt::handle &in_handle, const maya_exe_ns::replace_file_arg &in_arg);
  virtual ~maya_exe() override;

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void *data);
};
}  // namespace doodle
