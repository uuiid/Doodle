#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <vector>

namespace doodle {
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

class DOODLELIB_API qcloth_update_arg : public async_task {
 public:
  FSys::path alembic_file_dir_{};
  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  // from json
  friend void from_json(const nlohmann::json& in_json, qcloth_update_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const qcloth_update_arg& out_obj);

  boost::asio::awaitable<void> run() override;
};

}  // namespace doodle