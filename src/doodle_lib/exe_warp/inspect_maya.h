#pragma once
#include "doodle_core/core/core_set.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/signals2.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace doodle {

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

  std::shared_ptr<kitsu::kitsu_client> client_{};
  uuid task_id_{};
 inspect_file_arg() = default;
  explicit inspect_file_arg(const std::string& in_token, const uuid& in_task_id)
      : client_(std::make_shared<kitsu::kitsu_client>(core_set::get_set().server_ip)), task_id_(in_task_id) {
    client_->set_token(in_token);
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
}  // namespace doodle