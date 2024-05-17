//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/auto_light_render_video.h>
#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_method/computer.h>

#include "exe_warp/maya_exe.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
namespace doodle::http {

class websocket_sink_mt : public spdlog::sinks::base_sink<std::mutex> {
  http_work *work_{};

 public:
  explicit websocket_sink_mt(http_work *in_work) : work_(in_work) {}
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
    boost::asio::post(g_io_context(), [l_work = work_, level = msg.level, l_msg = fmt::to_string(formatted)]() {
      if (!l_work) return;
      if (!l_work->handle_) return;
      if (!l_work->handle_.any_of<http_websocket_data>()) return;
      auto &l_websocket = l_work->handle_.get<http_websocket_data>();
      l_websocket.seed(
          nlohmann::json{{"type", "logger"}, {"level", level}, {"task_id", l_work->task_info_.task_id_}, {"msg", l_msg}}
      );
    });
  }
  void flush_() override {}
};

void http_work::run(const std::string &in_server_address, std::uint16_t in_port) {
  timer_          = std::make_shared<timer>(g_io_context());
  server_address_ = in_server_address;
  port_           = in_port;

  g_ctx().emplace<maya_exe_ptr>(std::make_shared<maya_exe>());
  g_ctx().emplace<ue_exe_ptr>(std::make_shared<ue_exe>());
  core_set_init{}.config_to_user();
  app_base::Get().on_cancel.slot().assign([this](boost::asio::cancellation_type_t in_error_code) {
    if (handle_ && handle_.any_of<http_websocket_data>()) {
      handle_.get<http_websocket_data>().do_close();
    }
  });
  host_name_ = boost::asio::ip::host_name();
  do_connect();
}
void http_work::do_connect() {
  if (!handle_) {
    handle_ = entt::handle{*g_reg(), g_reg()->create()};
    handle_.emplace<http_websocket_data>();
    logger_ = handle_.emplace<socket_logger>().logger_;
  }
  handle_.get<http_websocket_data>().async_connect(
      server_address_, "v1/computer", port_,
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(),
          [this](boost::system::error_code in_error_code) {
            if (in_error_code) {
              logger_->log(log_loc(), level::err, "连接失败 {}", in_error_code);
              do_wait();
              return;
            }
            // handle_.get<http_websocket_data>().on_message.connect(
            //     std::bind(&http_work::read_task_info, this, std::placeholders::_1, std::placeholders::_2)
            // );
            is_connect_ = true;
            send_state();
          }
      )
  );
}
void http_work::do_wait() {
  logger_->log(log_loc(), level::info, "开始等待下一次心跳");
  timer_->expires_after(std::chrono::seconds{2});
  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [this](boost::system::error_code in_error_code) {
        if (in_error_code == boost::asio::error::operation_aborted) {
          return;
        }
        if (in_error_code) {
          logger_->log(log_loc(), level::err, "on_wait error: {}", in_error_code);
          return;
        }
        if (!handle_ || !is_connect_) {
          do_connect();
          return;
        }
        send_state();
      }
  ));
}

void http_work::send_state() {
  if (!handle_) {
    return;
  }
  if (task_info_.task_info_.is_null()) {
    handle_.get<http_websocket_data>().seed(
        nlohmann::json{{"type", "set_state"}, {"state", computer_status::free}, {"host_name", host_name_}}
    );
  } else {
    handle_.get<http_websocket_data>().seed(
        nlohmann::json{{"type", "set_state"}, {"state", computer_status::busy}, {"host_name", host_name_}}
    );
  }
  do_wait();
}
void http_work::read_task_info(const nlohmann::json &in_json, const entt::handle &in_handle) {
  if (!in_json.contains("id")) {
    logger_->log(log_loc(), level::err, "json parse error: {}", in_json.dump());
    return;
  }
  task_info_.task_id_   = in_json["id"].get<std::int32_t>();
  task_info_.task_info_ = in_json["data"];
  send_state();
  run_task();
}

void http_work::run_task() {
  if (task_info_.task_info_.is_null()) {
    return;
  }
  core_set_init{}.read_file();

  if (task_info_.task_info_.contains("type")) {
    auto l_type = task_info_.task_info_["type"].get<run_task_type>();
    switch (l_type) {
      case run_task_type::maya_exe: {
        logger_->log(log_loc(), level::info, "开始运行maya exe任务");
        end_task({});
        break;
      }
      case run_task_type::UE_exe: {
        logger_->log(log_loc(), level::info, "开始运行UE exe任务");
        end_task({});
        break;
      }
      case run_task_type::auto_light_task: {
        run_auto_light_task();
        break;
      }
    }
  }
}
void http_work::end_task(boost::system::error_code in_error_code) {
  if (in_error_code) {
    logger_->log(log_loc(), level::err, "任务执行失败: {}", in_error_code);
  } else {
    logger_->log(log_loc(), level::info, "任务执行成功");
  }
  if (!handle_) return;

  handle_.get<http_websocket_data>().seed(nlohmann::json{
      {"type", computer_websocket_fun::set_task},
      {"status", in_error_code ? server_task_info_status::failed : server_task_info_status::completed}
  });
  task_info_.task_id_   = 0;
  task_info_.task_info_ = nlohmann::json{};
  send_state();
}

void http_work::run_auto_light_task() {
  logger_->log(log_loc(), level::info, "开始运行自动灯光任务");

  if (!task_info_.task_info_.contains("file_path") || !task_info_.task_info_["file_path"].is_string() ||
      !task_info_.task_info_.contains("export_anim_time") || !task_info_.task_info_["export_anim_time"].is_number() ||
      !task_info_.task_info_.contains("episodes") || !task_info_.task_info_["episodes"].is_number() ||
      !task_info_.task_info_.contains("shot") || !task_info_.task_info_["shot"].is_number() ||
      !task_info_.task_info_.contains("project_name") || !task_info_.task_info_["project_name"].is_string() ||
      !task_info_.task_info_.contains("project_path") || !task_info_.task_info_["project_path"].is_string() ||
      !task_info_.task_info_.contains("project_en_str") || !task_info_.task_info_["project_en_str"].is_string() ||
      !task_info_.task_info_.contains("project_shor_str") || !task_info_.task_info_["project_shor_str"].is_string()) {
    logger_->log(log_loc(), level::err, "任务参数错误");
    end_task({ERROR_INVALID_PARAMETER, boost::system::system_category()});
    return;
  }

  FSys::path l_file_path{task_info_.task_info_["file_path"].get<std::string>()};

  if (!FSys::exists(l_file_path)) {
    logger_->log(log_loc(), level::err, "文件不存在 {}", l_file_path);
    end_task({ERROR_FILE_NOT_FOUND, boost::system::system_category()});
    return;
  }

  entt::handle l_msg{*g_reg(), g_reg()->create()};
  auto &l_process_message = l_msg.emplace<process_message>(l_file_path.filename().generic_string());
  l_process_message.logger()->sinks().emplace_back(std::make_shared<websocket_sink_mt>(this));
  l_msg.emplace<episodes>(task_info_.task_info_["episodes"].get<std::int32_t>());
  l_msg.emplace<shot>(
      task_info_.task_info_["shot"].get<std::int32_t>(),
      task_info_.task_info_.contains("shot_enum")
          ? magic_enum::enum_cast<shot::shot_ab_enum>(task_info_.task_info_["shot_enum"].get<std::string>())
                .value_or(shot::shot_ab_enum::None)
          : shot::shot_ab_enum::None
  );
  l_msg.emplace<project>(
      task_info_.task_info_["project_name"].get<std::string>(),
      task_info_.task_info_["project_path"].get<std::string>(),
      task_info_.task_info_["project_en_str"].get<std::string>(),
      task_info_.task_info_["project_shor_str"].get<std::string>(), ""
  );

  down_auto_light_anim_file l_down_anim_file{l_msg};
  import_and_render_ue l_import_and_render_ue{l_msg};
  auto_light_render_video l_auto_light_render_video{l_msg};
  up_auto_light_anim_file l_up_auto_light_file{l_msg};
  l_up_auto_light_file.async_end(boost::asio::bind_executor(
      g_io_context(),
      [this, l_msg](boost::system::error_code in_error_code, const FSys::path &in_path) {
        auto l_logger = l_msg.get<process_message>().logger();
        if (!in_error_code) l_logger->log(log_loc(), level::err, "任务输出 {}", in_path);
        end_task(in_error_code);
        l_logger->flush();
        boost::asio::post(g_io_context(), [l_msg]() {
          if (auto l_ = l_msg; l_) l_.destroy();
        });
      }
  ));
  l_auto_light_render_video.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_up_auto_light_file)));
  l_import_and_render_ue.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_auto_light_render_video)));
  l_down_anim_file.async_down_end(boost::asio::bind_executor(g_io_context(), std::move(l_import_and_render_ue)));

  if (task_info_.task_info_.contains("sim_path_list")) {
    auto l_arg             = maya_exe_ns::qcloth_arg{};
    l_arg.file_path        = task_info_.task_info_["file_path"].get<std::string>();
    l_arg.export_anim_time = task_info_.task_info_["export_anim_time"].get<std::int32_t>();
    l_arg.sim_path_list    = task_info_.task_info_["sim_path_list"].get<std::set<FSys::path>>();
    l_arg.bitset_          = maya_exe_ns::flags::k_export_abc_type | maya_exe_ns::flags::k_touch_sim_file |
                    maya_exe_ns::flags::k_create_play_blast | maya_exe_ns::flags::k_export_anim_file;
    if (task_info_.task_info_.contains("replace_ref_file") && task_info_.task_info_["replace_ref_file"].is_boolean() &&
        task_info_.task_info_["replace_ref_file"].get<bool>()) {
      l_arg.bitset_ |= maya_exe_ns::flags::k_replace_ref_file;
    }

    g_ctx().get<maya_exe_ptr>()->async_run_maya(
        l_msg, l_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file))
    );
  } else {
    auto l_arg             = maya_exe_ns::export_fbx_arg{};
    l_arg.file_path        = task_info_.task_info_["file_path"].get<std::string>();
    l_arg.export_anim_time = task_info_.task_info_["export_anim_time"].get<std::int32_t>();

    g_ctx().get<maya_exe_ptr>()->async_run_maya(
        l_msg, l_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file))
    );
  }
}
}  // namespace doodle::http