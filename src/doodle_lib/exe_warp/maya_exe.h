//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/core/wait_op.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>

#include <bitset>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <utility>

namespace doodle {
class maya_exe;

namespace maya_exe_ns {
namespace flags {
static constexpr std::bitset<8> k_replace_ref_file{0b1 << 0};
static constexpr std::bitset<8> k_sim_file{0b1 << 1};
static constexpr std::bitset<8> k_export_abc_type{0b1 << 2};
static constexpr std::bitset<8> k_export_fbx_type{0b1 << 3};
static constexpr std::bitset<8> k_create_play_blast{0b1 << 4};
static constexpr std::bitset<8> k_touch_sim_file{0b1 << 5};
static constexpr std::bitset<8> k_export_anim_file{0b1 << 6};  /// 安排导出动画文件, 针对解算使用
// 标准解算 00010111
// 触摸解算(自动灯光) 01110101
// 标准导出fbx 00001000
// static constexpr std::bitset<8> create_ref_file{0b1 << 0};
}  // namespace flags

class arg {
 public:
  arg()          = default;
  virtual ~arg() = default;
  FSys::path file_path{};
  std::int32_t t_post{};
  std::int32_t export_anim_time{};
  std::bitset<8> bitset_;
  FSys::path maya_path;
  // 输出结果路径
  FSys::path out_path_file_{};

  friend void to_json(nlohmann::json& in_nlohmann_json_j, const arg& in_nlohmann_json_t) {
    in_nlohmann_json_j["path"]             = in_nlohmann_json_t.file_path.generic_string();
    in_nlohmann_json_j["t_post"]           = in_nlohmann_json_t.t_post;
    in_nlohmann_json_j["export_anim_time"] = in_nlohmann_json_t.export_anim_time;
    in_nlohmann_json_j["bitset_"]          = in_nlohmann_json_t.bitset_;
    in_nlohmann_json_j["maya_path"]        = in_nlohmann_json_t.maya_path;
    in_nlohmann_json_j["out_path_file_"]   = in_nlohmann_json_t.out_path_file_;
  }

  friend void from_json(const nlohmann::json& in_nlohmann_json_j, arg& in_nlohmann_json_t) {
    in_nlohmann_json_j["path"].get_to(in_nlohmann_json_t.file_path);
    in_nlohmann_json_j["t_post"].get_to(in_nlohmann_json_t.t_post);
    in_nlohmann_json_j["export_anim_time"].get_to(in_nlohmann_json_t.export_anim_time);
    in_nlohmann_json_j["bitset_"].get_to(in_nlohmann_json_t.bitset_);
    in_nlohmann_json_j["maya_path"].get_to(in_nlohmann_json_t.maya_path);
    in_nlohmann_json_j["out_path_file_"].get_to(in_nlohmann_json_t.out_path_file_);
  }

  virtual std::string to_json_str() const = 0;
  virtual std::string get_arg() const     = 0;
};

class DOODLELIB_API qcloth_arg : public maya_exe_ns::arg {
 public:
  constexpr static std::string_view k_name{"cloth_sim_config"};
  FSys::path sim_path{};

  friend void to_json(nlohmann::json& nlohmann_json_j, const qcloth_arg& nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg&>(nlohmann_json_t));
    nlohmann_json_j["sim_path"] = nlohmann_json_t.sim_path;
  };

  friend void from_json(const nlohmann::json& nlohmann_json_j, qcloth_arg& nlohmann_json_t) {
    from_json(nlohmann_json_j, dynamic_cast<arg&>(nlohmann_json_t));
    nlohmann_json_j["sim_path"].get_to(nlohmann_json_t.sim_path);
  };

  std::string to_json_str() const override {
    nlohmann::json l_json{};
    l_json = *this;
    return l_json.dump();
  }

  std::string get_arg() const override { return "cloth_sim_config"; }
};

class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool use_all_ref;
  bool rig_file_export;
  constexpr static std::string_view k_name{"export_fbx_config"};

  friend void to_json(nlohmann::json& nlohmann_json_j, const export_fbx_arg& nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg&>(nlohmann_json_t));
    nlohmann_json_j["use_all_ref"] = nlohmann_json_t.use_all_ref;
    nlohmann_json_j["rig_file_export"] = nlohmann_json_t.rig_file_export;
  };

  friend void from_json(const nlohmann::json& in_nlohmann_json_j, export_fbx_arg& in_nlohmann_json_t) {
    from_json(in_nlohmann_json_j, dynamic_cast<arg&>(in_nlohmann_json_t));
    in_nlohmann_json_j["use_all_ref"].get_to(in_nlohmann_json_t.use_all_ref);
    in_nlohmann_json_j["rig_file_export"].get_to(in_nlohmann_json_t.rig_file_export);
  }

  std::string to_json_str() const override {
    nlohmann::json l_json{};
    l_json = *this;
    return l_json.dump();
  }

  std::string get_arg() const override { return "export_fbx_config"; }
};

class DOODLELIB_API replace_file_arg : public maya_exe_ns::arg {
 public:
  std::vector<std::pair<FSys::path, FSys::path>> file_list{};
  constexpr static std::string_view k_name{"replace_file_config"};

  friend void to_json(nlohmann::json& nlohmann_json_j, const replace_file_arg& nlohmann_json_t) {
    to_json(nlohmann_json_j, dynamic_cast<const arg&>(nlohmann_json_t));
    nlohmann_json_j["file_list"] = nlohmann_json_t.file_list;
  }

  friend void from_json(const nlohmann::json& in_nlohmann_json_j, replace_file_arg& in_nlohmann_json_t) {
    from_json(in_nlohmann_json_j, dynamic_cast<arg&>(in_nlohmann_json_t));
    in_nlohmann_json_j["file_list"].get_to(in_nlohmann_json_t.file_list);
  }

  std::string to_json_str() const override {
    nlohmann::json l_json{};
    l_json = *this;
    return l_json.dump();
  }

  std::string get_arg() const override { return "replace_file_config"; }
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
} // namespace doodle