//
// Created by TD on 2026-11-10.
//

#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/face_capture_transcode.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include <spdlog/async.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <spdlog/details/log_msg.h>
#include <string>
#include <vector>

#include "face_capture.h"

namespace doodle::http::local {

namespace {

using uuid_t = boost::uuids::uuid;

FSys::path session_dir_of(const uuid_t& in_session_id) {
  return face_capture::capture_session::default_root() / fmt::format("{}", in_session_id);
}

template <class Mutex>
class face_capture_transcode_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<server_task_info> task_info_{};
  std::once_flag flag_;

 public:
  explicit face_capture_transcode_sink(std::shared_ptr<server_task_info> in_task_info)
      : task_info_(std::move(in_task_info)) {}

  void sink_it_(const spdlog::details::log_msg&) override {
    std::call_once(flag_, [this]() { set_state(); });
    task_info_->progress_ += 0.001;
    if (task_info_->progress_ > 1.0) task_info_->progress_ = std::rand() % 100 / 500.0;
    if (static_cast<int>(task_info_->progress_ * 100) % 2 == 0 || task_info_->progress_ <= 0.1)
      socket_io::broadcast(
          socket_io::local_server_task_info_progress_broadcast_t{
              .id_ = task_info_->uuid_id_, .progress_ = task_info_->progress_
          }
      );
  }
  void flush_() override {}

  void set_state() {
    if (task_info_->status_ != server_task_info_status::submitted) return;

    task_info_->status_   = server_task_info_status::running;
    task_info_->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *task_info_});
  }
};

using face_capture_transcode_sink_mt = face_capture_transcode_sink<std::mutex>;

void start_task(
    const std::shared_ptr<face_capture_transcode_tasks>& in_tasks,
    const std::shared_ptr<face_capture_transcode_task>& in_task
) {
  const auto& l_task_info = in_task->task_info_;

  auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                       fmt::format("{}.log", l_task_info->uuid_id_);
  auto l_logger      = std::make_shared<spdlog::async_logger>(
      l_task_info->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
      spdlog::thread_pool()
  );
  l_logger->sinks().emplace_back(std::make_shared<face_capture_transcode_sink_mt>(l_task_info));
  in_task->task_->set_logger(l_logger);

  l_task_info->status_   = server_task_info_status::running;
  l_task_info->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_task_info});

  boost::asio::co_spawn(
      g_io_context(),
      [in_task, in_tasks]() -> boost::asio::awaitable<void> {
        const auto& l_info = in_task->task_info_;
        try {
          co_await in_task->task_->run();
          l_info->status_   = server_task_info_status::completed;
          l_info->progress_ = 1.0;
        } catch (const std::exception& in_e) {
          l_info->status_        = server_task_info_status::failed;
          l_info->last_line_log_ = in_e.what();
        }
        l_info->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
        socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_info});
        in_tasks->finish(l_info->uuid_id_);
      }(),
      boost::asio::detached
  );
}

}  // namespace

std::shared_ptr<face_capture_transcode_task> face_capture_transcode_tasks::enqueue(
    std::shared_ptr<async_task> in_task, std::shared_ptr<server_task_info> in_task_info
) {
  auto l_ptr        = std::make_shared<face_capture_transcode_task>();
  l_ptr->task_      = std::move(in_task);
  l_ptr->task_info_ = std::move(in_task_info);

  std::lock_guard<std::mutex> _(mtx_);
  tasks_.emplace_back(l_ptr);
  return l_ptr;
}

std::shared_ptr<face_capture_transcode_task> face_capture_transcode_tasks::find(const uuid_t& in_preview_id) const {
  std::lock_guard<std::mutex> _(mtx_);
  auto l_itr = std::ranges::find_if(tasks_, [&in_preview_id](const auto& in_ptr) {
    return in_ptr && in_ptr->task_info_ && in_ptr->task_info_->uuid_id_ == in_preview_id;
  });
  if (l_itr == tasks_.end()) return nullptr;
  return *l_itr;
}

std::vector<std::shared_ptr<face_capture_transcode_task>> face_capture_transcode_tasks::find_all() const {
  std::lock_guard<std::mutex> _(mtx_);
  return tasks_;
}

void face_capture_transcode_tasks::start_next() {
  std::shared_ptr<face_capture_transcode_task> l_ptr{};
  {
    std::lock_guard<std::mutex> _(mtx_);
    if (running_.has_value()) return;
    auto l_itr = std::ranges::find_if(tasks_, [](const auto& in_ptr) {
      return in_ptr && in_ptr->task_info_ && in_ptr->task_info_->status_ == server_task_info_status::submitted;
    });
    if (l_itr == tasks_.end()) return;
    l_ptr    = *l_itr;
    running_ = l_ptr->task_info_->uuid_id_;
  }
  start_task(shared_from_this(), l_ptr);
}

void face_capture_transcode_tasks::finish(const uuid_t& in_preview_id) {
  {
    std::lock_guard<std::mutex> _(mtx_);
    if (running_ == in_preview_id) running_ = std::nullopt;
  }
  start_next();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_transcode, post) {
  DOODLE_CHICK_HTTP(FSys::exists(session_dir_of(preview_id_) / "take.mp4"), not_found, "take.mp4 不存在");

  auto l_json = in_handle->get_json();
  DOODLE_CHICK_HTTP(
      l_json.contains("ue_project") && l_json.at("ue_project").is_string(), unprocessable_entity, "缺少 ue_project"
  );
  auto const l_ue_project = FSys::path{l_json.at("ue_project").get<std::string>()};
  DOODLE_CHICK_HTTP(FSys::exists(l_ue_project), unprocessable_entity, "ue_project 不存在: {}", l_ue_project.generic_string());

  auto l_client = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);

  auto l_arg           = std::make_shared<face_capture_transcode_arg>();
  l_arg->preview_id_   = preview_id_;
  l_arg->ue_project_   = l_ue_project;
  l_arg->kitsu_client_ = l_client;

  auto l_info          = std::make_shared<server_task_info>();
  l_info->uuid_id_     = preview_id_;
  l_info->name_        = fmt::format("face_capture_transcode_{}", preview_id_);
  l_info->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_info->command_     = nlohmann::json{} = *l_arg;

  auto l_ptr = tasks_->enqueue(l_arg, l_info);
  tasks_->start_next();
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = *l_ptr->task_info_);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_transcode, get) {
  auto l_ptr = tasks_->find(preview_id_);
  DOODLE_CHICK_HTTP(l_ptr != nullptr, not_found, "该会话没有转码任务");
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = *l_ptr->task_info_);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_transcode_tasks_all, get) {
  auto l_list = tasks_->find_all();
  std::vector<server_task_info> l_result{};
  l_result.reserve(l_list.size());
  for (const auto& l_ptr : l_list) l_result.emplace_back(*l_ptr->task_info_);
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_result);
}

}  // namespace doodle::http::local
