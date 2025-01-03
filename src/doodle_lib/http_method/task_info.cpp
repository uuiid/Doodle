//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include "doodle_core/core/app_base.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "computer_reg_data.h"
#include "exe_warp/import_and_render_ue.h"
#include "exe_warp/maya_exe.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<void> task_emit(const std::shared_ptr<server_task_info>& in_ptr) {
  auto l_computer_list = computer_reg_data_manager::get().list();
  if (l_computer_list.empty()) co_return;
  for (auto&& l_com : l_computer_list) {
    if (l_com->computer_data_ptr_->uuid_id_ == in_ptr->run_computer_id_)
      if (auto l_c = l_com->client.lock(); l_c) {
        co_await l_c->async_write_websocket(
            nlohmann::json{{"type", doodle_config::work_websocket_event::post_task}, {"id", in_ptr->uuid_id_}}.dump()
        );
        co_return;
      }
  }
}

boost::asio::awaitable<boost::beast::http::message_generator> post_task(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr  = std::make_shared<server_task_info>();

  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  l_json.get_to(*l_ptr);
  l_ptr->uuid_id_     = core_set::get_set().get_uuid();
  l_ptr->submit_time_ = chrono::sys_time_pos::clock::now();

  if (l_ptr->exe_.empty())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "运行程序任务为空");

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<computer>(l_ptr->run_computer_id_); l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到计算机");

  co_await g_ctx().get<sqlite_database>().install(l_ptr);

  boost::asio::co_spawn(g_io_context(), task_emit(l_ptr), boost::asio::consign(boost::asio::detached, l_ptr));
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task(session_data_ptr in_handle) {
  uuid l_uuid = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id"));

  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(l_uuid); !l_list.empty())
    co_return in_handle->make_msg((nlohmann::json{} = l_list[0]).dump());

  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "任务不存在");
}

boost::asio::awaitable<boost::beast::http::message_generator> list_task(session_data_ptr in_handle) {
  std::optional<server_task_info_type> l_type{};
  for (auto&& i : in_handle->url_.params()) {
    if (i.has_value && i.key == "type") {
      l_type = magic_enum::enum_cast<server_task_info_type>(i.value);
    }
  }
  if (l_type) {
    if (auto l_list = g_ctx().get<sqlite_database>().get_server_task_info_by_type(*l_type); !l_list.empty())
      co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
    co_return in_handle->make_msg("[]"s);
  }

  co_return in_handle->make_msg((nlohmann::json{} = g_ctx().get<sqlite_database>().get_all<server_task_info>()).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task_logger(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id"))};

  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_uuid);
  if (!FSys::exists(l_path))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志不存在");
  auto l_mime = std::string{kitsu::mime_type(l_path.extension())};
  l_mime += "; charset=utf-8";
  auto l_ex = in_handle->make_msg(l_path, l_mime);
  if (!l_ex) co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_ex.error());
  co_return std::move(*l_ex);
}

boost::asio::awaitable<boost::beast::http::message_generator> delete_task(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>(boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id")));

  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(*l_uuid);
      !l_list.empty() && l_list.front().status_ == server_task_info_status::running) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::method_not_allowed, "任务正在运行中, 无法删除"
    );
  }
  co_await g_ctx().get<sqlite_database>().remove<server_task_info>(l_uuid);
  co_return in_handle->make_msg("{}");
}

template <class Mutex>
class run_post_task_local_impl_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<server_task_info> task_info_{};
  std::once_flag flag_;

 public:
  explicit run_post_task_local_impl_sink(std::shared_ptr<server_task_info> in_task_info) : task_info_(in_task_info) {}
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // std::call_once(flag_, &set_state, this);
    std::call_once(flag_, [this]() { set_state(); });
  }
  void flush_() override {}
  void set_state() {
    task_info_->status_   = server_task_info_status::running;
    task_info_->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().install(task_info_), boost::asio::detached);
  }
};
using run_post_task_local_impl_sink_mt = run_post_task_local_impl_sink<std::mutex>;

class run_post_task_local_cancel_manager {
  std::map<uuid, std::size_t> sigs_index;
  std::vector<std::shared_ptr<boost::asio::cancellation_signal>> sigs_{};
  std::recursive_mutex mtx_;

 public:
  run_post_task_local_cancel_manager() = default;

  boost::asio::cancellation_slot add(uuid in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    auto itr = std::find_if(sigs_.begin(), sigs_.end(), [](std::shared_ptr<boost::asio::cancellation_signal>& sig) {
      return !sig->slot().has_handler();
    });
    if (itr != sigs_.end()) {
      sigs_index[in_id] = std::distance(sigs_.begin(), itr);
      return (*itr)->slot();
    } else {
      sigs_index[in_id] = sigs_.size();
      return sigs_.emplace_back(std::make_shared<boost::asio::cancellation_signal>())->slot();
    }
  }

  void cancel(uuid in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    if (auto itr = sigs_index.find(in_id);
        itr != sigs_index.end() && itr->second < sigs_.size() && sigs_[itr->second]->slot().has_handler())
      sigs_[itr->second]->emit(boost::asio::cancellation_type::all);
  }
  void cancel_all() {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    for (auto& sig : sigs_) sig->emit(boost::asio::cancellation_type::all);
  }
};

boost::asio::awaitable<void> run_post_task_local_impl(
    std::shared_ptr<server_task_info> in_task_info, std::shared_ptr<maya_exe_ns::arg> in_arg, logger_ptr in_logger
) {
  in_logger->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(in_task_info));
  auto [l_e, l_r]         = co_await async_run_maya(in_arg, in_logger);
  in_task_info->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  // 用户取消
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
    in_task_info->status_ = server_task_info_status::canceled;
  else if (l_e) {
    in_task_info->status_ = server_task_info_status::failed;
  } else
    in_task_info->status_ = server_task_info_status::completed;
  boost::asio::co_spawn(
      g_io_context(), g_ctx().get<sqlite_database>().install(in_task_info),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::detached

      )
  );
  co_return;
}
boost::asio::awaitable<void> run_post_task_local_impl(
    std::shared_ptr<server_task_info> in_task_info, std::shared_ptr<import_and_render_ue_ns::args> in_arg,
    logger_ptr in_logger
) {
  in_logger->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(in_task_info));
  auto [l_e, l_r]         = co_await async_auto_loght(in_arg, in_logger);
  in_task_info->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  // 用户取消
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
    in_task_info->status_ = server_task_info_status::canceled;
  else if (l_e) {
    in_task_info->status_ = server_task_info_status::failed;
  } else
    in_task_info->status_ = server_task_info_status::completed;
  boost::asio::co_spawn(
      g_io_context(), g_ctx().get<sqlite_database>().install(in_task_info),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::detached

      )
  );
  co_return;
}
boost::asio::awaitable<boost::beast::http::message_generator> post_task_local(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr = std::make_shared<server_task_info>();
  std::shared_ptr<maya_exe_ns::arg> l_arg{};
  std::shared_ptr<import_and_render_ue_ns::args> l_import_and_render_args{};
  logger_ptr l_logger_ptr{};

  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  l_json.get_to(*l_ptr);
  l_ptr->uuid_id_         = core_set::get_set().get_uuid();
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);

  auto& l_task = l_json["task_data"];
  if (l_task.contains("replace_ref_file")) {
    auto l_arg_t = std::make_shared<maya_exe_ns::qcloth_arg>();
    l_task.get_to(*l_arg_t);
    l_arg_t->sim_path = FSys::path{l_task["project"]["path"].get<std::string>()} / "6-moxing" / "CFX";

    l_arg             = l_arg_t;
  } else if (l_task.contains("file_list")) {
    auto l_arg_t = std::make_shared<maya_exe_ns::replace_file_arg>();
    l_task.get_to(*l_arg_t);
    l_arg = l_arg_t;
  } else if (l_task.contains("is_sim")) {
    l_import_and_render_args = std::make_shared<import_and_render_ue_ns::args>();
    l_task.get_to(*l_import_and_render_args);
    if (l_task["is_sim"].get<bool>()) {
      auto l_arg_t = std::make_shared<maya_exe_ns::qcloth_arg>();
      l_task.get_to(*l_arg_t);
      l_arg_t->sim_path           = l_import_and_render_args->project_.path_ / "6-moxing" / "CFX";
      l_arg_t->export_file        = true;
      l_arg_t->touch_sim          = true;
      l_arg_t->export_anim_file   = true;
      l_arg_t->create_play_blast_ = true;
      l_arg                       = l_arg_t;
    } else {
      auto l_arg_t = std::make_shared<maya_exe_ns::export_fbx_arg>();
      l_task.get_to(*l_arg_t);
      l_arg_t->create_play_blast_ = true;
      l_arg                       = l_arg_t;
    }
    l_import_and_render_args->maya_arg_ = l_arg;
  } else {
    auto l_arg_t = std::make_shared<maya_exe_ns::export_fbx_arg>();
    l_task.get_to(*l_arg_t);
    l_arg = l_arg_t;
  }

  auto l_logger_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_ptr->uuid_id_);
  l_logger_ptr = std::make_shared<spdlog::async_logger>(
      l_ptr->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
      spdlog::thread_pool()
  );

  co_await g_ctx().get<sqlite_database>().install(l_ptr);
  if (l_import_and_render_args) {
    boost::asio::co_spawn(
        g_io_context(), run_post_task_local_impl(l_ptr, l_import_and_render_args, l_logger_ptr),
        boost::asio::bind_cancellation_slot(
            g_ctx().get<run_post_task_local_cancel_manager>().add(l_ptr->uuid_id_),
            boost::asio::consign(boost::asio::detached, l_arg, l_ptr, l_logger_ptr)

        )
    );
  } else {
    boost::asio::co_spawn(
        g_io_context(), run_post_task_local_impl(l_ptr, l_arg, l_logger_ptr),
        boost::asio::bind_cancellation_slot(
            g_ctx().get<run_post_task_local_cancel_manager>().add(l_ptr->uuid_id_),
            boost::asio::consign(boost::asio::detached, l_arg, l_ptr, l_logger_ptr)

        )
    );
  }

  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> patch_task_local(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>(boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id")));
  auto l_server_task_info = std::make_shared<server_task_info>();

  server_task_info l_server_task_info_org{};
  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(*l_uuid); l_list.empty()) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "任务不存在");
  } else {
    *l_server_task_info    = l_list[0];
    l_server_task_info_org = l_list[0];
  }
  auto l_sr   = l_server_task_info->status_;
  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  if (l_json.contains("status")) l_json["status"].get_to(l_server_task_info->status_);
  if (l_json.contains("name")) l_json["name"].get_to(l_server_task_info->name_);
  if (*l_server_task_info != l_server_task_info_org)
    co_await g_ctx().get<sqlite_database>().install(l_server_task_info);
  if (l_sr == server_task_info_status::running && l_server_task_info->status_ == server_task_info_status::canceled)
    g_ctx().get<run_post_task_local_cancel_manager>().cancel(l_server_task_info->uuid_id_);
  co_return in_handle->make_msg((nlohmann::json{} = *l_server_task_info).dump());
}

}  // namespace

void task_info_reg(doodle::http::http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
void task_info_reg_local(doodle::http::http_route& in_route) {
  if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
  g_ctx().emplace<run_post_task_local_cancel_manager>();
  app_base::Get().on_stop.connect([]() { g_ctx().get<run_post_task_local_cancel_manager>().cancel_all(); });

  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task_local))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::patch, "api/doodle/task/{id}", patch_task_local))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
}  // namespace doodle::http