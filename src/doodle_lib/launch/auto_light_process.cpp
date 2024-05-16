//
// Created by TD on 2024/3/22.
//

#include "auto_light_process.h"

#include "doodle_core/configure/static_value.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/platform/win/register_file_type.h>

#include "doodle_app/lib_warp/imgui_warp.h"
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/core/auto_light_render_video.h>
#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/gui/widgets/render_monitor.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <wil/result.h>
namespace doodle::launch {

// 映射网络驱动器
bool map_network_drive(const std::string &in_drive, const std::string &in_path) {
  auto l_drive = conv::utf_to_utf<wchar_t>(in_drive);
  auto l_path  = conv::utf_to_utf<wchar_t>(in_path);

  NETRESOURCEW l_net_resource{};  // 网络资源
  l_net_resource.dwType       = RESOURCETYPE_DISK;
  l_net_resource.lpLocalName  = l_drive.data();
  l_net_resource.lpRemoteName = l_path.data();

  auto l_ret                  = WNetAddConnection2W(&l_net_resource, nullptr, nullptr, CONNECT_TEMPORARY);
  if (l_ret != NO_ERROR) {
    LOG_LAST_ERROR();
    default_logger_raw()->error("映射网络驱动器失败: {}", l_ret);
    return false;
  }
  return true;
}

bool auto_light_process_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  auto l_sink        = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  using work_guard_t = decltype(boost::asio::make_work_guard(g_io_context()));
  auto l_work_guard  = std::make_shared<work_guard_t>(boost::asio::make_work_guard(g_io_context()));

  using signal_t     = boost::asio::signal_set;
  auto signal_       = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  signal_->async_wait([](boost::system::error_code in_error_code, int in_sig) {
    if (in_error_code) {
      if (!app_base::GetPtr()->is_stop())
        default_logger_raw()->log(log_loc(), level::err, "信号错误: {}", in_error_code.message());
      return;
    }
    default_logger_raw()->log(log_loc(), level::warn, "收到信号 {} {}", in_error_code.message(), in_sig);
    app_base::GetPtr()->stop_app(1);
  });
  in_vector.emplace_back(signal_);
  g_logger_ctrl().add_log_sink(l_sink, "auto_light_process");
  core_set_init l_core_set_init{};
  l_core_set_init.config_to_user();
  l_core_set_init.read_file();

  // 开始映射网络驱动器
  std::vector<std::pair<std::string, std::string>> l_drive_list{
      {"V:", R"(\\192.168.10.250\public\DuBuXiaoYao_3)"},
      {"R:", R"(\\192.168.10.240\public\WanGuShenHua)"},
      {"U:", R"(\\192.168.10.218\WanYuFengShen)"}
  };
  for (auto &l_drive : l_drive_list) {
    if (!FSys::exists(l_drive.first))
      if (!map_network_drive(l_drive.first, l_drive.second)) return true;
  }

  if (in_arh[g_only_map_drive]) {
    return true;
  }

  if (!in_arh(g_maya_file)) {
    default_logger_raw()->error("必须有maya文件");
    return true;
  }
  auto l_maya       = g_ctx().emplace<maya_exe_ptr>(std::make_shared<maya_exe>());
  FSys::path l_file = FSys::from_quotation_marks(in_arh(g_maya_file).str());
  auto l_strm       = l_file.stem().generic_string();
  if (!FSys::exists(l_file)) {
    default_logger_raw()->error("文件不存在: {}", l_file.string());
    return true;
  }

  std::int32_t l_export_anim_time = 1001;
  if (in_arh(g_export_anim_time)) {
    try {
      l_export_anim_time = std::stoi(in_arh(g_export_anim_time).str());
    } catch (const std::exception &e) {
      default_logger_raw()->error("导出动画时间解析错误: {}", e.what());
    }
  }

  episodes l_episodes{};
  shot l_shot{};
  project l_project{};
  l_episodes.analysis(l_file);
  l_shot.analysis(l_file);
  auto l_prj_list = register_file_type::get_project_list();
  auto l_it = ranges::find_if(l_prj_list, [&](const project &in_prj) { return l_strm.starts_with(in_prj.p_shor_str); });
  if (l_it != l_prj_list.end()) {
    l_project = *l_it;
  } else {
    default_logger_raw()->error("未找到项目: {}", l_strm);
    return true;
  }

  entt::handle l_msg{*g_reg(), g_reg()->create()};
  auto &l_process_message = l_msg.emplace<process_message>(l_file.filename().generic_string());
  l_process_message.logger()->sinks().emplace_back(l_sink);
  l_msg.emplace<episodes>(l_episodes);
  l_msg.emplace<shot>(l_shot);
  l_msg.emplace<project>(l_project);

  down_auto_light_anim_file l_down_anim_file{l_msg};
  import_and_render_ue l_import_and_render_ue{l_msg};
  auto_light_render_video l_auto_light_render_video{l_msg};
  up_auto_light_anim_file l_up_auto_light_file{l_msg};
  l_up_auto_light_file.async_end(boost::asio::bind_executor(
      g_io_context(),
      [l_msg, l_work_guard](boost::system::error_code in_error_code, std::filesystem::path in_path) mutable {
        l_work_guard.reset();
        if (in_error_code) {
          l_msg.get<process_message>().set_state(process_message::state::fail);
          app_base::GetPtr()->stop_app(in_error_code.value());
        } else {
          default_logger_raw()->info("成功");
          l_msg.get<process_message>().set_state(process_message::state::success);
          app_base::GetPtr()->stop_app();
        }
      }
  ));
  l_auto_light_render_video.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_up_auto_light_file)));
  l_import_and_render_ue.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_auto_light_render_video)));
  l_down_anim_file.async_down_end(boost::asio::bind_executor(g_io_context(), std::move(l_import_and_render_ue)));

  if (in_arh[g_animation]) {
    auto l_arg             = maya_exe_ns::export_fbx_arg{};
    l_arg.file_path        = l_file;
    l_arg.export_anim_time = l_export_anim_time;

    l_maya->async_run_maya(l_msg, l_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file)));
  } else if (in_arh[g_cfx]) {
    //    auto l_arg             = maya_exe_ns::export_fbx_arg{};
    //    l_arg.file_path        = l_file;
    //    l_arg.export_anim_time = l_export_anim_time;
    //    l_arg.bitset_ |= maya_exe_ns::flags::k_export_abc_type;
    //    l_maya->async_run_maya(l_msg, l_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file)));
  } else {
    default_logger_raw()->error("必须有参数");
    return true;
  }
  return false;
}
}  // namespace doodle::launch
