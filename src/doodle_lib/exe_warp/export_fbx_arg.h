#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle {

/**
 * @brief fbx导出
 */
class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool create_play_blast_{};
  bool rig_file_export_{};
  std::double_t film_aperture_{};
  image_size size_{};

  FSys::path maya_file_{};
  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

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
}  // namespace doodle