#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <filesystem>

namespace doodle {

/**
 * @brief fbx导出
 */
class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool create_play_blast_{};
  bool only_upload_{};
  std::double_t film_aperture_{};
  image_size size_{};
  FSys::path maya_file_{};
  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  constexpr static std::string_view k_name{"export_fbx"};

  struct get_export_fbx_arg {
    std::double_t film_aperture_{};
    image_size size_{};
    FSys::path movie_file_{};

    // to json
    friend void to_json(nlohmann::json& j, const get_export_fbx_arg& p) {
      j["film_aperture"] = p.film_aperture_;
      j["image_size"]    = p.size_;
      j["movie_file"]    = p.movie_file_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, get_export_fbx_arg& p) {
      j.at("film_aperture").get_to(p.film_aperture_);
      j.at("image_size").get_to(p.size_);
      j.at("movie_file").get_to(p.movie_file_);
    }
  };

  // form json
  friend void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj);
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  boost::asio::awaitable<void> run() override;
};

/**
 * @brief fbx导出
 */
class DOODLELIB_API export_fbx_arg_epiboly : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"export_fbx"};
  bool create_play_blast_{};
  std::double_t film_aperture_{};
  image_size size_{};
  // form json
  friend void from_json(const nlohmann::json& in_json, export_fbx_arg_epiboly& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const export_fbx_arg_epiboly& out_obj);
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
  boost::asio::awaitable<void> run() override;
};

}  // namespace doodle