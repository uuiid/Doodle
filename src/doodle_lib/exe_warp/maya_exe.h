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
  struct out_file_t {
    // 输出文件
    FSys::path out_file{};
    // 引用文件
    FSys::path ref_file{};
    // 需要隐藏的材质列表
    std::vector<std::string> hide_material_list{};

    friend void from_json(const nlohmann::json& nlohmann_json_j, out_file_t& nlohmann_json_t);

    friend void to_json(nlohmann::json& nlohmann_json_j, const out_file_t& nlohmann_json_t);
  };

  std::uint32_t begin_time{};
  std::uint32_t end_time{};
  std::vector<out_file_t> out_file_list{};

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

class DOODLELIB_API qcloth_arg : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"cloth_sim"};
  FSys::path sim_path{};
  bool replace_ref_file;
  bool sim_file;
  bool export_file;  // 导出abc文件
  bool touch_sim;
  bool export_anim_file;
  bool create_play_blast_{};
  std::double_t film_aperture_{};
  image_size size_{};

  // form json
  friend void from_json(const nlohmann::json& in_json, qcloth_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const qcloth_arg& out_obj);

  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  boost::asio::awaitable<void> run() override;
};
/**
 * @brief fbx导出
 */
class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool create_play_blast_{};
  bool rig_file_export_{};
  std::double_t film_aperture_{};
  image_size size_{};

  constexpr static std::string_view k_name{"export_fbx"};

  // form json
  friend void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj);
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  boost::asio::awaitable<void> run() override;
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

enum class inspect_file_type { model_maya };
NLOHMANN_JSON_SERIALIZE_ENUM(inspect_file_type, {{inspect_file_type::model_maya, "model_maya"}});
/**
 * @brief 检查文件
 * 使用 run 检查时, 会自动将 maya 文件向上一级移动, 并备份文件
 * 使用 async_run_maya 检查时, 不会移动文件
 */
class DOODLELIB_API inspect_file_arg : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"inspect_file"};
  bool surface_5_{};  // 是否检测5边面
  /// 重命名检查
  bool rename_check_{};
  /// 名称长度检查
  bool name_length_check_{};
  /// 模型历史,数值,检查
  bool history_check_{};
  /// 特殊复制检查
  bool special_copy_check_{};
  /// uv正反面检查
  bool uv_check_{};
  /// 模型k帧检查
  bool kframe_check_{};
  /// 空间名称检查
  bool space_name_check_{};
  /// 只有默认相机检查
  bool only_default_camera_check_{};
  /// 多余点数检查
  bool too_many_point_check_{};
  /// 多 UV 检查
  bool multi_uv_inspection_{};

  inspect_file_arg() {
    surface_5_ = rename_check_ = name_length_check_ = history_check_ = special_copy_check_ = uv_check_ = kframe_check_ =
        space_name_check_ = only_default_camera_check_ = too_many_point_check_ = multi_uv_inspection_ = true;
  }

  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  // form json
  friend void from_json(const nlohmann::json& in_json, inspect_file_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const inspect_file_arg& out_obj);

  boost::asio::awaitable<void> run() override;
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