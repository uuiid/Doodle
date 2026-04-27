#include "doodle_core/metadata/task.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "doodle_lib/core/app_base.h"
#include "doodle_lib/core/global_function.h"
#include "doodle_lib/core/socket_io/broadcast.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/this_coro.hpp>

#include "http_method/kitsu.h"
#include "reg.h"
#include "sqlite_orm/sqlite_select_data.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/opencv.hpp>
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
#ifdef DOODLE_SEED2
    const auto l_task_status = co_await in_client->query_task(in_task->task_id_);
    if (!l_task_status.contains("status")) {
      default_logger_raw()->error("查询任务状态失败，响应内容: {}", l_task_status.dump());
      continue;
    }
    auto l_status = l_task_status.at("status").get<sd2::task_status>();
    if (l_status == sd2::task_status::succeeded || l_status == sd2::task_status::failed ||
        l_status == sd2::task_status::expired || l_status == sd2::task_status::cancelled) {
      in_task->status_        = l_status;
      in_task->data_response_ = l_task_status;
      co_await get_sqlite_database().update(in_task);
      break;
    }
#else
    break;
#endif
  }
#ifdef DOODLE_SEED2

  if (in_task->status_ == sd2::task_status::succeeded && in_task->data_response_.contains("content") &&
      in_task->data_response_.at("content").contains("video_url")) {
    auto l_video_url      = in_task->data_response_.at("content").at("video_url").get<std::string>();
    auto l_file           = co_await in_client->download_result(l_video_url);
    auto l_file_picture   = g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_task_file(in_task->uuid_id_, ".mp4");
    auto l_file_thumbnail = g_ctx().get<kitsu_ctx_t>().get_sd2_thumbnail_task_file(in_task->uuid_id_);
    if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    {
      // 生成预览文件
      auto l_video = cv::VideoCapture{l_file.generic_string()};
      // 读取第一帧生成预览文件
      cv::Mat l_image{};
      l_video >> l_image;
      if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});
      auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));

      if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
      cv::imwrite(l_file_thumbnail.generic_string(), l_image);
    }
    FSys::rename(l_file, l_file_picture);
  }
#else
  in_task->status_ = sd2::task_status::succeeded;
  co_await get_sqlite_database().update(in_task);
#endif

  socket_io::broadcast(
      socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task->uuid_id_, .status_ = in_task->status_}
  );
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, post) {
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_task = std::make_shared<sd2::task>();
  l_json.get_to(*l_task);
  l_task->user_id_        = person_.person_.uuid_id_;
  l_task->ai_studio_id_   = person_.get_ai_studio_id();
  l_task->file_extension_ = ".mp4";
  auto l_client           = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);
  auto l_studio           = l_sql.get_by_uuid<ai_studio>(l_task->ai_studio_id_);
  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
#ifdef DOODLE_SEED2
  l_task->task_id_ = co_await l_client->run_task(l_task->data_request_);  // 异步运行任务，不等待结果
#endif
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
  co_return in_handle->make_msg(nlohmann::json{} = sqlite_select::get_sd2_tasks_for_person(person_.person_.uuid_id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get) {
  co_return in_handle->make_msg(
      nlohmann::json{} = sqlite_select::get_sd2_tasks_for_ai_studio(person_.get_ai_studio_id())
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  co_return in_handle->make_msg(l_task);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, put) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")
  auto l_studio = l_sql.get_by_uuid<ai_studio>(l_task.ai_studio_id_);
  auto l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);

  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
  if (l_task->status_ == sd2::task_status::running && !l_task->task_id_.empty())
    co_await l_client->cancel_task(l_task.task_id_);
  l_task.status_ = sd2::task_status::cancelled;

  co_return in_handle->make_msg(nlohmann::json{} = l_task);
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