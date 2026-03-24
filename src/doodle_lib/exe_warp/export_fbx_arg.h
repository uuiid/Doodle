#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <cstdint>
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
  std::int32_t frame_in_{};
  std::int32_t frame_out_{};

  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  constexpr static std::string_view k_name{"export_fbx"};

  struct get_export_fbx_arg {
    std::double_t film_aperture_{};
    image_size size_{};
    FSys::path movie_file_{};
    std::int32_t frame_in_{};
    std::int32_t frame_out_{};
    FSys::path maya_file_name_{};

    // to json
    friend void to_json(nlohmann::json& j, const get_export_fbx_arg& p) {
      j["film_aperture"]  = p.film_aperture_;
      j["image_size"]     = p.size_;
      j["movie_file"]     = p.movie_file_;
      j["frame_in"]       = p.frame_in_;
      j["frame_out"]      = p.frame_out_;
      j["maya_file_name"] = p.maya_file_name_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, get_export_fbx_arg& p) {
      j.at("film_aperture").get_to(p.film_aperture_);
      j.at("image_size").get_to(p.size_);
      j.at("movie_file").get_to(p.movie_file_);
      j.at("frame_in").get_to(p.frame_in_);
      j.at("frame_out").get_to(p.frame_out_);
      j.at("maya_file_name").get_to(p.maya_file_name_);
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

class DOODLELIB_API export_fbx_arg_distributed : public export_fbx_arg {
 public:
  struct args {
    bool create_play_blast_{true};
    std::double_t film_aperture_{};
    image_size size_{};
    std::int32_t frame_in_{};
    std::int32_t frame_out_{};
    FSys::path maya_file_name_{};
    uuid task_id_{};
    // from json
    friend void from_json(const nlohmann::json& j, args& p);
    // to json
    friend void to_json(nlohmann::json& j, const args& p);
  };

 private:
  args arg_;

 public:
  export_fbx_arg_distributed()           = default;
  ~export_fbx_arg_distributed() override = default;
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  void set_arg(const nlohmann::json& in_json);
  constexpr static std::string_view k_name{"export_fbx"};
  // form json
  friend void from_json(const nlohmann::json& in_json, export_fbx_arg_distributed& out_obj);
  // to json
  friend void to_json(nlohmann::json& in_json, const export_fbx_arg_distributed& out_obj);
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