#include "doodle_core/metadata/task.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/project.h"
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/scope/scope_exit.hpp>

#include "http_method/kitsu.h"
#include "reg.h"
#include <chrono>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/opencv.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#define DOODLE_SEED2

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

namespace {

auto get_sd2_tasks_for_person(const uuid& in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace orm;
  return select(l_sql)
      .columns(object<sd2::task>())
      .from<sd2::task>()
      .where(c(&sd2::task::user_id_) == in_person_id && !c(&sd2::task::archived_))()
      .to_vector();
}

void video_create_picture(const FSys::path& in_video_path, const uuid& in_id) {
  auto l_file_picture   = g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(in_id, ".mp4");
  auto l_file_thumbnail = g_ctx().get<kitsu_ctx_t>().get_sd2_thumbnail_file(in_id);
  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  {
    // 生成预览文件
    auto l_video = cv::VideoCapture{in_video_path.generic_string()};
    // 读取第一帧生成预览文件
    cv::Mat l_image{};
    l_video >> l_image;
    if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});
    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));

    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }
  FSys::rename(in_video_path, l_file_picture);
}

class seedance2_task_run_manager {
  struct task_info {
    explicit task_info(const sd2::task& in_task, const std::string& in_app_secret)
        : task_(in_task), app_secret_(in_app_secret) {}
    sd2::task task_;
    std::string app_secret_;
  };
  struct seedance2_info {
    std::int64_t completion_tokens_{0};
    sd2::task_status status_{sd2::task_status::queued};
    nlohmann::json data_response_{};
    uuid preview_file_{};
  };

  std::atomic_bool is_running_{false};

  std::vector<task_info> get_task() {
    auto l_sql = get_sqlite_database();
    using namespace orm;
    return select(l_sql)
        .columns(object<sd2::task>(), &ai_studio::app_secret_)
        .from<sd2::task>()
        .where(c(&sd2::task::status_) == sd2::task_status::queued)
        .left_outer_join<ai_studio>(&ai_studio::uuid_id_, &sd2::task::ai_studio_id_)()
        .to_vector<task_info>();
  }

  boost::asio::awaitable<void> async_run() {
    boost::scope::scope_exit on_exit{[this]() { is_running_ = false; }};
    boost::asio::steady_timer l_timer{g_io_context()};
    std::map<std::string, std::shared_ptr<seedance2_client>> l_client_map;
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      for (auto&& l_task_info : get_task()) {
        std::shared_ptr<seedance2_client> l_client;
        if (l_client_map.contains(l_task_info.app_secret_)) {
          l_client = l_client_map[l_task_info.app_secret_];
        } else {
          l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);
          l_client->set_token(l_task_info.app_secret_);
          l_client->set_logger(g_logger_ctrl().get_http());
          l_client_map[l_task_info.app_secret_] = l_client;
        }
        co_await query_task_and_down(l_task_info.task_, l_client);
      }

      l_timer.expires_after(5s);
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
  }

  boost::asio::awaitable<void> query_task_and_down(
      const sd2::task& in_task, const std::shared_ptr<seedance2_client>& in_client
  ) try {
    const auto l_task_info     = co_await in_client->query_task(in_task.task_id_);
    auto l_task_ptr            = std::make_shared<sd2::task>(in_task);
    l_task_ptr->data_response_ = l_task_info;
    const sd2::task_status l_status{
        l_task_info.contains("status") ? l_task_info.at("status").get<sd2::task_status>() : sd2::task_status::failed
    };
    auto l_sql = get_sqlite_database();

    switch (l_status) {
      case sd2::task_status::queued:
        co_return;
      case sd2::task_status::running: {
        l_task_ptr->status_ = l_status;
        co_await l_sql.update(l_task_ptr);
        socket_io::broadcast(
            socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_task_ptr->status_}
        );
        co_return;
      }
      case sd2::task_status::succeeded: {
        if (l_task_info.contains("usage") && l_task_info.at("usage").contains("completion_tokens")) {
          l_task_ptr->completion_tokens_ = l_task_info.at("usage").at("completion_tokens").get<std::int64_t>();
        }
        if (l_task_info.contains("content") && l_task_info.at("content").contains("video_url")) {
          auto l_video_url = l_task_info.at("content").at("video_url").get<std::string>();
          SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "任务 {} 完成，下载视频 {}", in_task.uuid_id_, l_video_url);
          auto l_file                = co_await in_client->download_result(l_video_url);
          auto l_preview_file        = std::make_shared<sd2::ai_preview_file>();
          l_preview_file->extension_ = ".mp4";
          co_await l_sql.install(l_preview_file);
          l_task_ptr->preview_file_ = l_preview_file->uuid_id_;
          video_create_picture(l_file, l_preview_file->uuid_id_);
        }
        break;
      }
      case sd2::task_status::cancelled:
      case sd2::task_status::failed:
      case sd2::task_status::expired:
        break;
    }

    l_task_ptr->status_   = l_status;
    l_task_ptr->ended_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    co_await l_sql.update(l_task_ptr);

    if (l_status == sd2::task_status::succeeded && l_task_ptr->completion_tokens_ > 0) {
      // 为负数时, 如果任务成功，说明实际消耗的 token 比预估的少，返还差值
      co_await add_remaining_tokens_for_person(
          in_task.user_id_, in_task.completion_tokens_ - l_task_ptr->completion_tokens_
      );
    } else {
      // 任务失败或者其他状态，返还 token
      co_await add_remaining_tokens_for_person(in_task.user_id_, in_task.completion_tokens_);
    }
    socket_io::broadcast(
        socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_task_ptr->status_}
    );
  } catch (...) {
    auto l_err_str = boost::current_exception_diagnostic_information();
    SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), l_err_str);
  }

 public:
  static seedance2_task_run_manager& Get() {
    static seedance2_task_run_manager instance;
    return instance;
  }

  void run() {
    if (is_running_.exchange(true)) return;
    boost::asio::co_spawn(
        g_io_context(), async_run(),
        boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
    );
  }
};

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::task>())
                      .from<sd2::task>()
                      .where(c(&sd2::task::ai_generate_entity_id_) == entity_id_)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task, post) {
  if (get_remaining_tokens_for_person(person_.person_.uuid_id_) - doodle_config::g_max_task_completion_tokens <= 0)
    throw_exception(doodle_error{"当周可用token数量不足，请联系管理员"});

  auto l_task = std::make_shared<sd2::task>();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

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
  co_await add_remaining_tokens_for_person(person_.person_.uuid_id_, -l_task->completion_tokens_);
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
  seedance2_task_run_manager::Get().run();

  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, put) {
  auto l_sql    = get_sqlite_database();
  auto l_task   = l_sql.get_by_uuid<sd2::task>(id_);
  auto l_studio = l_sql.get_by_uuid<ai_studio>(person_.get_ai_studio_id());
  auto l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);

  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
  if (!l_task.task_id_.empty()) co_await l_client->cancel_task(l_task.task_id_);
  l_task.status_ = sd2::task_status::cancelled;

  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, delete_) {
  auto l_sql        = get_sqlite_database();
  auto l_task       = std::make_shared<sd2::task>(l_sql.get_by_uuid<sd2::task>(id_));
  l_task->archived_ = true;
  co_await l_sql.update(l_task);
  co_return in_handle->make_msg(nlohmann::json{{"id", id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_thumbnail, get) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_thumbnail_file(id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "缩略图不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_pictures, get) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_pictures_file(id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "图片不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}

}  // namespace doodle::http::seedance2