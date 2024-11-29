//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/core/wait_op.h>

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
  FSys::path maya_path{};

  virtual std::vector<std::string> get_arg_list()      = 0;
  virtual void parse_args(const argh::parser& in_argh) = 0;
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
  FSys::path out_path_file_{};

  std::vector<std::string> get_arg_list() {
    std::vector<std::string> l_args{
        fmt::format("--{}", k_name),                           //
        fmt::format("--path={}", file_path.generic_string()),  //
        fmt::format("--sim_path={}", sim_path.generic_string())
    };
    if (replace_ref_file) l_args.emplace_back("--replace_ref_file");
    if (create_play_blast_) l_args.emplace_back("--create_play_blast");
    if (sim_file) l_args.emplace_back("--sim_file");
    if (export_file) l_args.emplace_back("--export_file");
    if (touch_sim) l_args.emplace_back("--touch_sim");
    if (export_anim_file) l_args.emplace_back("--export_anim_file");

    return l_args;
  }
  virtual void parse_args(const argh::parser& in_argh) {
    file_path          = in_argh({"path"}).str();
    sim_path           = in_argh({"sim_path"}).str();
    sim_file           = in_argh[{"sim_file"}];
    export_file        = in_argh[{"export_file"}];
    touch_sim          = in_argh[{"touch_sim"}];
    export_anim_file   = in_argh[{"export_anim_file"}];
    create_play_blast_ = in_argh[{"create_play_blast"}];
    maya_path          = in_argh({"maya_path"}).str();
    out_path_file_     = in_argh({"out_path_file"}).str();
    replace_ref_file   = in_argh[{"replace_ref_file"}];
  }
};

class DOODLELIB_API export_fbx_arg : public maya_exe_ns::arg {
 public:
  bool create_play_blast_{};
  FSys::path out_path_file_{};
  bool rig_file_export_{};

  constexpr static std::string_view k_name{"export_fbx"};

  std::vector<std::string> get_arg_list() {
    std::vector<std::string> l_args{
        fmt::format("--{}", k_name),  //
        fmt::format("--path={}", file_path.generic_string())
    };
    if (create_play_blast_) l_args.emplace_back("--create_play_blast");
    if (rig_file_export_) l_args.emplace_back("--rig_file_export");
    return l_args;
  }
  virtual void parse_args(const argh::parser& in_argh) {
    file_path          = in_argh({"path"}).str();
    create_play_blast_ = in_argh[{"create_play_blast"}];
    rig_file_export_   = in_argh[{"rig_file_export"}];
  }
};

class DOODLELIB_API replace_file_arg : public maya_exe_ns::arg {
 public:
  std::vector<std::pair<FSys::path, FSys::path>> file_list{};
  constexpr static std::string_view k_name{"replace_file"};

  std::vector<std::string> get_arg_list() {
    std::vector<std::string> l_args{
        fmt::format("--{}", k_name),  //
        fmt::format("--path={}", file_path.generic_string())
    };
    return l_args;
  }
  virtual void parse_args(const argh::parser& in_argh) {
    file_path = in_argh({"path"}).str();
    maya_path = in_argh({"maya_path"}).str();
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
} // namespace doodle