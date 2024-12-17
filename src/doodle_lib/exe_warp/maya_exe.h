//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/core/wait_op.h>
#include <doodle_core/metadata/image_size.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>

#include <argh.h>
#include <bitset>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <utility>

namespace doodle {
class maya_exe;

namespace maya_exe_ns {

class arg {
 public:
  arg()          = default;
  virtual ~arg() = default;

  FSys::path file_path{};
  FSys::path out_path_file_{};

  // form json
  friend void from_json(const nlohmann::json& in_json, arg& out_obj) {
    if (in_json.contains("path")) in_json.at("path").get_to(out_obj.file_path);
    if (in_json.contains("out_path_file")) in_json.at("out_path_file").get_to(out_obj.out_path_file_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const arg& out_obj) {
    in_json["path"]          = out_obj.file_path.generic_string();
    in_json["out_path_file"] = out_obj.out_path_file_.generic_string();
  }
  virtual std::tuple<std::string, std::string> get_json_str() = 0;
};

class DOODLELIB_API qcloth_arg : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"cloth_sim"};
  FSys::path sim_path{};
  bool replace_ref_file;
  bool sim_file;
  bool export_file;
  bool touch_sim;
  bool export_anim_file;
  bool create_play_blast_{};
  std::double_t film_aperture_{};
  image_size size_{};

  // form json
  friend void from_json(const nlohmann::json& in_json, qcloth_arg& out_obj) {
    from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));
    if (in_json.contains("sim_path")) in_json.at("sim_path").get_to(out_obj.sim_path);
    if (in_json.contains("replace_ref_file")) in_json.at("replace_ref_file").get_to(out_obj.replace_ref_file);
    if (in_json.contains("sim_file")) in_json.at("sim_file").get_to(out_obj.sim_file);
    if (in_json.contains("export_file")) in_json.at("export_file").get_to(out_obj.export_file);
    if (in_json.contains("touch_sim")) in_json.at("touch_sim").get_to(out_obj.touch_sim);
    if (in_json.contains("export_anim_file")) in_json.at("export_anim_file").get_to(out_obj.export_anim_file);
    if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
    if (in_json.contains("camera_film_aperture"))
      in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
    else
      out_obj.film_aperture_ = 1.78;
    if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const qcloth_arg& out_obj) {
    to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));
    in_json["sim_path"]          = out_obj.sim_path.generic_string();
    in_json["replace_ref_file"]  = out_obj.replace_ref_file;
    in_json["sim_file"]          = out_obj.sim_file;
    in_json["export_file"]       = out_obj.export_file;
    in_json["touch_sim"]         = out_obj.touch_sim;
    in_json["export_anim_file"]  = out_obj.export_anim_file;
    in_json["create_play_blast"] = out_obj.create_play_blast_;
    in_json["camera_film_aperture"]     = out_obj.film_aperture_;
    in_json["image_size"]        = out_obj.size_;
  }

  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
};

class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool create_play_blast_{};
  bool rig_file_export_{};
  std::double_t film_aperture_{};
  image_size size_{};

  constexpr static std::string_view k_name{"export_fbx"};

  // form json
  friend void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj) {
    from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

    if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
    if (in_json.contains("out_path_file")) in_json.at("out_path_file").get_to(out_obj.out_path_file_);
    if (in_json.contains("rig_file_export")) in_json.at("rig_file_export").get_to(out_obj.rig_file_export_);
    if (in_json.contains("camera_film_aperture"))
      in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
    else
      out_obj.film_aperture_ = 1.78;
    if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj) {
    to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

    in_json["create_play_blast"] = out_obj.create_play_blast_;
    in_json["out_path_file"]     = out_obj.out_path_file_.generic_string();
    in_json["rig_file_export"]   = out_obj.rig_file_export_;
    in_json["camera_film_aperture"]     = out_obj.film_aperture_;
    in_json["image_size"]        = out_obj.size_;
  }
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
};

class DOODLELIB_API replace_file_arg : public maya_exe_ns::arg {
 public:
  std::vector<std::pair<FSys::path, FSys::path>> file_list{};
  constexpr static std::string_view k_name{"replace_file"};

  // form json
  friend void from_json(const nlohmann::json& in_json, replace_file_arg& out_obj) {
    from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

    if (in_json.contains("file_list")) in_json.at("file_list").get_to(out_obj.file_list);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const replace_file_arg& out_obj) {
    to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

    in_json["file_list"] = out_obj.file_list;
  }
  std::tuple<std::string, std::string> get_json_str() override {
    return std::tuple<std::string, std::string>{k_name, (nlohmann::json{} = *this).dump()};
  }
};

struct maya_out_arg {
  struct out_file_t {
    // 输出文件
    FSys::path out_file{};
    // 引用文件
    FSys::path ref_file{};

    friend void from_json(const nlohmann::json& nlohmann_json_j, out_file_t& nlohmann_json_t) {
      nlohmann_json_j["out_file"].get_to(nlohmann_json_t.out_file);
      nlohmann_json_j["ref_file"].get_to(nlohmann_json_t.ref_file);
    };

    friend void to_json(nlohmann::json& nlohmann_json_j, const out_file_t& nlohmann_json_t) {
      nlohmann_json_j["out_file"] = nlohmann_json_t.out_file.generic_string();
      nlohmann_json_j["ref_file"] = nlohmann_json_t.ref_file.generic_string();
    };
  };

  std::uint32_t begin_time{};
  std::uint32_t end_time{};
  std::vector<out_file_t> out_file_list{};

  friend void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg& nlohmann_json_t) {
    nlohmann_json_j["begin_time"].get_to(nlohmann_json_t.begin_time);
    nlohmann_json_j["end_time"].get_to(nlohmann_json_t.end_time);
    nlohmann_json_j["out_file_list"].get_to(nlohmann_json_t.out_file_list);
  };

  friend void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg& nlohmann_json_t) {
    nlohmann_json_j["begin_time"]    = nlohmann_json_t.begin_time;
    nlohmann_json_j["end_time"]      = nlohmann_json_t.end_time;
    nlohmann_json_j["out_file_list"] = nlohmann_json_t.out_file_list;
  };
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

boost::asio::awaitable<std::tuple<boost::system::error_code, maya_exe_ns::maya_out_arg>> async_run_maya(
    std::shared_ptr<maya_exe_ns::arg> in_arg, logger_ptr in_logger
);
}  // namespace doodle