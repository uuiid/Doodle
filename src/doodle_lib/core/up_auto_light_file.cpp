//
// Created by TD on 2024/1/9.
//

#include "up_auto_light_file.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>

#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/thread_copy_io.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>

namespace doodle {
void up_auto_light_anim_file::init() { data_->logger_ = msg_.get<process_message>().logger(); }
void up_auto_light_anim_file::operator()(boost::system::error_code in_error_code, FSys::path in_gen_path) const {
  if (in_error_code) {
    data_->logger_->error("上传自动灯光文件失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->logger_->info("开始上传文件夹 :{}", in_gen_path.generic_string());
  // 场景名称
  auto l_u_project = msg_.get<down_auto_light_anim_file::down_info>().render_project_;
  auto l_scene     = l_u_project.parent_path();
  auto l_rem_path  = msg_.get<project>().p_path / "03_Workflow" / doodle_config::ue4_shot /
                    fmt::format("EP{:04}", msg_.get<episodes>().p_episodes) / l_u_project.stem();
  data_->out_file_path_ = l_rem_path;

  // maya输出
  auto l_maya_out       = msg_.get<maya_exe_ns::maya_out_arg>().out_file_list |
                    ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t &in_arg) {
                      return in_arg.out_file.parent_path();
                    }) |
                    ranges::views::filter([](const FSys::path &in_path) { return !in_path.empty(); }) |
                    ranges::to_vector;
  l_maya_out |= ranges::actions::unique;
  std::vector<std::pair<FSys::path, FSys::path>> l_up_file_list{};
  // 渲染输出文件
  l_up_file_list.emplace_back(in_gen_path, l_rem_path / in_gen_path.lexically_proximate(l_scene));
  // 渲染工程文件
  l_up_file_list.emplace_back(l_scene / doodle_config::ue4_config, l_rem_path / doodle_config::ue4_config);
  l_up_file_list.emplace_back(l_scene / doodle_config::ue4_content, l_rem_path / doodle_config::ue4_content);
  l_up_file_list.emplace_back(l_u_project, l_rem_path / l_u_project.filename());
  // maya输出文件
  for (const auto &l_maya : l_maya_out) {
    l_up_file_list.emplace_back(l_maya, l_rem_path.parent_path() / l_maya.stem());
  }
  // 开始正式上传
  g_ctx().get<thread_copy_io_service>().async_copy_old(
      l_up_file_list, FSys::copy_options::recursive, boost::asio::bind_executor(g_io_context(), std::move(*this))
  );
}
void up_auto_light_anim_file::operator()(
    boost::system::error_code in_error_code, auto_light_render_video::video_path_t in_gen_path
) const {
  if (in_error_code) {
    data_->logger_->error("上传自动灯光文件失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->logger_->info("开始上传合成视屏 :{}", in_gen_path.render_video_path_.generic_string());

  // ue输出
  auto l_ue_out = msg_.get<import_and_render_ue::render_out_path_t>().path;
  data_->logger_->log(log_loc(), level::info, "开始上传文件夹 :{}", l_ue_out);

  // 场景名称
  auto l_u_project = msg_.get<down_auto_light_anim_file::down_info>().render_project_;
  auto l_scene     = l_u_project.parent_path();
  auto l_rem_path  = msg_.get<project>().p_path / "03_Workflow" / doodle_config::ue4_shot /
                    fmt::format("EP{:04}", msg_.get<episodes>().p_episodes) / l_u_project.stem();
  data_->out_file_path_ = l_rem_path;

  // maya输出
  auto l_maya_out       = msg_.get<maya_exe_ns::maya_out_arg>().out_file_list |
                    ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t &in_arg) {
                      return in_arg.out_file.parent_path();
                    }) |
                    ranges::views::filter([](const FSys::path &in_path) { return !in_path.empty(); }) |
                    ranges::to_vector;
  l_maya_out |= ranges::actions::unique;
  std::vector<std::pair<FSys::path, FSys::path>> l_up_file_list{};
  // 渲染输出文件
  l_up_file_list.emplace_back(l_ue_out, l_rem_path / l_ue_out.lexically_proximate(l_scene));
  // 渲染工程文件
  l_up_file_list.emplace_back(l_scene / doodle_config::ue4_config, l_rem_path / doodle_config::ue4_config);
  l_up_file_list.emplace_back(l_scene / doodle_config::ue4_content, l_rem_path / doodle_config::ue4_content);
  l_up_file_list.emplace_back(l_u_project, l_rem_path / l_u_project.filename());
  // maya输出文件
  for (const auto &l_maya : l_maya_out) {
    l_up_file_list.emplace_back(l_maya, l_rem_path.parent_path() / l_maya.stem());
  }
  // 合成的视屏文件
  l_up_file_list.emplace_back(
      in_gen_path.render_video_path_, l_rem_path.parent_path() / "mov" / in_gen_path.render_video_path_.filename()
  );
  // 开始正式上传
  g_ctx().get<thread_copy_io_service>().async_copy_old(
      l_up_file_list, FSys::copy_options::recursive, boost::asio::bind_executor(g_io_context(), std::move(*this))
  );
}
void up_auto_light_anim_file::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::err, "上传自动灯光文件失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  set_info_(data_->out_file_path_);
  wait_op_->ec_ = in_error_code;
  wait_op_->complete();
}
}  // namespace doodle