#include "doodle_core/metadata/task.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/this_coro.hpp>

#include "core/app_base.h"
#include "core/global_function.h"
#include "core/socket_io/broadcast.h"
#include "http_method/kitsu.h"
#include "reg.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <spdlog/spdlog.h>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

namespace {
constexpr static std::string_view g_sd2_host_url{"https://ark.cn-beijing.volces.com"};
boost::asio::awaitable<void> run_task(std::shared_ptr<sd2::task> in_task, std::shared_ptr<seedance2_client> in_client) {
  // 每隔5s 查询一次任务状态，直到任务完成或者失败
  boost::asio::steady_timer l_timer{g_io_context()};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    l_timer.expires_after(5s);
    co_await l_timer.async_wait(boost::asio::use_awaitable);
    const auto l_task_status = co_await in_client->query_task(in_task->task_id_);
    if (!l_task_status.contains("status")) {
      default_logger_raw()->error("查询任务状态失败，响应内容: {}", l_task_status.dump());
      continue;
    }
    auto l_status = l_task_status.at("status").get<sd2::task_status>();
    switch (l_status) {
      case sd2::task_status::queued:
      case sd2::task_status::running:
        SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "任务 {} 仍在进行中...", in_task->task_id_);
        break;
      case sd2::task_status::succeeded:
        SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "任务 {} 已完成!", in_task->task_id_);
      case sd2::task_status::failed:
        SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "任务 {} 失败了!", in_task->task_id_);
        socket_io::broadcast(
            socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task->uuid_id_, .status_ = l_status}
        );
        break;
      default:
        SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_http(), "未知的任务状态: {}", l_status);
        break;
    }
  }
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, post) {
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_task = std::make_shared<sd2::task>();
  l_json.get_to(*l_task);
  l_task->user_id_      = person_.person_.uuid_id_;
  l_task->ai_studio_id_ = person_.get_ai_studio_id();
  auto l_client         = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);
  auto l_studio         = l_sql.get_by_uuid<ai_studio>(l_task->ai_studio_id_);
  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
  l_task->task_id_ = co_await l_client->run_task(l_task->data_request_);  // 异步运行任务，不等待结果

  // 查找 以https://或者http://开头的url，并替换host部分为空
  static std::regex l_url_regex(R"(https?:\/\/[^\/\s]+)");
  for (auto&& l_value : l_task->data_request_.at("content")) {
    nlohmann::json* l_url{};
    if (l_value.contains("image_url"))
      l_url = &l_value.at("image_url").at("url");
    else if (l_value.contains("video_url"))
      l_url = &l_value.at("video_url").at("url");
    else if (l_value.contains("audio_url "))
      l_url = &l_value.at("audio_url ").at("url");
    else
      continue;

    *l_url = std::regex_replace(l_url->get<std::string>(), l_url_regex, "");
  }

  co_await l_sql.install(l_task);
  boost::asio::co_spawn(
      g_io_context(), run_task(l_task, l_client),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, l_client, l_task)
      )
  );  // 后台运行任务，不等待结果

  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::user_id_) == person_.person_.uuid_id_));
  co_return in_handle->make_msg(l_vec);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec =
      l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::ai_studio_id_) == person_.get_ai_studio_id()));
  co_return in_handle->make_msg(l_vec);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  co_return in_handle->make_msg(l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, delete_) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  co_await l_sql.remove<sd2::task>(id_);
  co_return in_handle->make_msg(nlohmann::json{{"id", id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_thumbnail_task, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_thumbnail_task_file(id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "缩略图不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_pictures_task, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_pictures_task_file(id_, l_task.file_extension_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "图片或者视频不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
}  // namespace doodle::http::seedance2