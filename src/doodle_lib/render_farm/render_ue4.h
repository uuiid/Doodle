//
// Created by td_main on 2023/8/4.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/render_ue4_arg.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <nlohmann/json.hpp>
namespace doodle::render_farm {
namespace detail {

using ue_server_id = doodle::detail::entt_id<entt::tag<"ue_server_id"_hs>>;

class render_ue4 {
 public:
  using arg_t = ue4_arg;

  explicit render_ue4(entt::handle in_msg, arg_t in_arg) : arg_(std::move(in_arg)), self_handle_(std::move(in_msg)) {
    set_meg();
  }
  ~render_ue4() = default;
  void run();

 private:
  arg_t arg_;

  entt::handle self_handle_;
  FSys::path manifest_path_;
  FSys::path loc_out_file_path_;
  FSys::path server_file_path;
  process_child_ptr child_ptr_;

  boost::asio::any_io_executor strand_;
  void set_meg();

  // 下载文件
  bool download_file(const FSys::path& in_file_path);

  bool updata_file();
  // 给服务器发送状态
  void send_server_state();

  void end_run();
  // 生成命令行
  [[nodiscard("")]] std::string generate_command_line() const;
  void run_impl(bool in_r);

  void do_read_log();
  void do_read_err();
};

}  // namespace detail
using render_ue4     = detail::render_ue4;
using render_ue4_ptr = std::shared_ptr<render_ue4>;
}  // namespace doodle::render_farm
