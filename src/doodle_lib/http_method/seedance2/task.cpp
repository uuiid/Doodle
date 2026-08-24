#include "doodle_core/metadata/task.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/seedance2/ai_episode.h"
#include "doodle_core/metadata/seedance2/ai_generate_entity.h"
#include "doodle_core/metadata/seedance2/subproject.h"
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core/core_set.h"
#include "core/http_client_core.h"
#include "http_method/kitsu.h"
#include "reg.h"
#include "sqlite_orm/orm/column_operations.h"
#include "sqlite_orm/orm/insert.h"
#include "sqlite_orm/orm/select.h"
#include <chrono>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/opencv.hpp>
#include <rapidfuzz/rapidfuzz_all.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>
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

  std::atomic_bool is_running_{false};

  std::vector<task_info> get_task() {
    auto l_sql = get_sqlite_database();
    using namespace orm;
    return select(l_sql)
        .columns(object<sd2::task>(), &ai_studio::app_secret_)
        .from<sd2::task>()
        .where(
            c(&sd2::task::status_) == sd2::task_status::queued || c(&sd2::task::status_) == sd2::task_status::running
        )
        .left_outer_join<ai_studio>(&ai_studio::uuid_id_, &sd2::task::ai_studio_id_)()
        .to_vector<task_info>();
  }

  boost::asio::awaitable<void> async_run() {
    boost::scope::scope_exit on_exit{[this]() { is_running_ = false; }};
    boost::asio::steady_timer l_timer{g_io_context()};
    std::map<std::string, std::shared_ptr<seedance2_client>> l_client_map;
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      auto l_tasks = get_task();
      if (l_tasks.empty()) co_return;
      for (auto&& l_task_info : l_tasks) {
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
    auto l_task_ptr = std::make_shared<sd2::task>(in_task);
    try {
      const auto l_task_info     = co_await in_client->query_task(in_task.task_id_);
      l_task_ptr->data_response_ = l_task_info;
    } catch (const doodle_error& in_err) {
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_main_error(), "查询任务 {} 失败, 错误: {}", in_task.uuid_id_, in_err.what()
      );
      l_task_ptr->status_        = sd2::task_status::failed;
      l_task_ptr->data_response_ = in_err.what();
    }
    const sd2::task_status l_status{
        l_task_ptr->data_response_.contains("status") ? l_task_ptr->data_response_.at("status").get<sd2::task_status>()
                                                      : sd2::task_status::failed
    };
    auto l_sql = get_sqlite_database();

    switch (l_status) {
      case sd2::task_status::queued:
        co_return;
      case sd2::task_status::running: {
        if (l_task_ptr->status_ != l_status) {
          l_task_ptr->status_ = l_status;
          co_await l_sql.update(l_task_ptr);
        }
        socket_io::broadcast(
            socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_task_ptr->status_}
        );
        co_return;
      }
      case sd2::task_status::succeeded: {
        if (l_task_ptr->data_response_.contains("usage") &&
            l_task_ptr->data_response_.at("usage").contains("completion_tokens")) {
          l_task_ptr->completion_tokens_ =
              l_task_ptr->data_response_.at("usage").at("completion_tokens").get<std::int64_t>();
        }
        if (l_task_ptr->data_response_.contains("content") &&
            l_task_ptr->data_response_.at("content").contains("video_url")) {
          auto l_video_url = l_task_ptr->data_response_.at("content").at("video_url").get<std::string>();
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
      co_await l_sql.run_sql(add_remaining_tokens_for_person(
          l_sql, in_task.user_id_, in_task.completion_tokens_ - l_task_ptr->completion_tokens_
      ));
    } else {
      // 任务失败或者其他状态，返还 token
      co_await l_sql.run_sql(add_remaining_tokens_for_person(l_sql, in_task.user_id_, in_task.completion_tokens_));
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
  person_.check_subproject_access(subproject_id_);
  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::task>())
                      .from<sd2::task>()
                      .where(c(&sd2::task::ai_generate_entity_id_) == entity_id_)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}
namespace {
// 获取 ip , 访问 http://ip.sb, 返回值就是 ip, 不是json字段
boost::asio::awaitable<std::string> get_self_ip() {
  using http_client_t     = doodle::http::http_client_ssl;
  using http_client_ptr_t = std::shared_ptr<http_client_ssl>;
  auto l_client           = std::make_shared<http_client_t>("https://api.ip.sb", *core_set::get_set().ctx_ptr);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/ip", 11};
  l_req.set(boost::beast::http::field::host, l_client->server_ip_and_port_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  // 3 次重试
  for (int i = 0; i < 3; ++i) {
    boost::beast::http::response<boost::beast::http::string_body> l_res{};
    try {
      co_await l_client->read_and_write(l_req, l_res, boost::asio::use_awaitable);
      if (l_res.result() != boost::beast::http::status::ok)
        throw_exception(doodle_error{"get_self_ip error {} {}", l_res.result(), l_res.body()});
      auto l_ip = l_res.body();
      if (l_ip.ends_with('\n')) l_ip.pop_back();
      co_return l_ip;
    } catch (const boost::system::system_error& e) {
      if (e.code() == boost::asio::error::operation_aborted) throw;
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), "get_self_ip error: {}", e.what());
      if (i == 2) throw;
    } catch (...) {
      auto l_err_str = boost::current_exception_diagnostic_information();
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), "get_self_ip error: {}", l_err_str);
      if (i == 2) throw;
    }
  }
  co_return std::string{};  // unreachable
}
// 将传入的 req 中的资源路径附加上服务器的 ip 地址，返回新的 req
nlohmann::json add_ip_to_req(const nlohmann::json& in_req, const std::string& in_ip) {
  nlohmann::json l_req = in_req;
  for (auto&& l_value : l_req.at("content")) {
    if (l_value.contains("image_url")) {
      auto& l_url = l_value.at("image_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    } else if (l_value.contains("video_url")) {
      auto& l_url = l_value.at("video_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    } else if (l_value.contains("audio_url")) {
      auto& l_url = l_value.at("audio_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    }
  }
  return l_req;
}

// 对比传入的任务和一批任务的相似度
std::vector<sd2::task_similarity> compare_task_similarity(
    const sd2::task& in_task, const std::vector<std::tuple<uuid, std::string>>& in_tasks
) {
  std::vector<sd2::task_similarity> l_result;
  l_result.reserve(in_tasks.size());
  auto l_cache = rapidfuzz::fuzz::CachedWRatio(in_task.text_prompt_);  // 缓存 in_task 的文本，提升性能
  for (const auto& [l_uuid, l_text_prompt] : in_tasks) {
    auto l_similarity = l_cache.similarity(l_text_prompt);
    if (l_similarity < 75) continue;  // 相似度小于 75 的任务不考虑
    l_result.emplace_back(
        sd2::task_similarity{
            .task_id_         = in_task.uuid_id_,
            .similar_task_id_ = l_uuid,
            .similarity_      = l_similarity,
        }
    );
  }
  return l_result;
}
std::vector<sd2::task_similarity> get_task_similarity_for_person(
    const decltype(get_sqlite_database())& l_sql, const sd2::task& in_task
) {
  using namespace orm;
  // 只检查最近 50 条任务
  auto l_tasks = select(l_sql)
                     .columns(&sd2::task::uuid_id_, &sd2::task::text_prompt_)
                     .from<sd2::task>()
                     .where(c(&sd2::task::user_id_) == in_task.user_id_ && !c(&sd2::task::archived_))
                     .order_by(&sd2::task::created_at_, false)
                     .limit(50)()
                     .to_vector();
  return compare_task_similarity(in_task, l_tasks);
}

}  // namespace
seedance2_subproject_task::seedance2_subproject_task() { seedance2_task_run_manager::Get().run(); }
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task, post) {
  person_.check_subproject_access(subproject_id_);

  if (get_remaining_tokens_for_person(person_.person_.uuid_id_) - doodle_config::g_max_task_completion_tokens <= 0)
    throw_exception(doodle_error{"当周可用token数量不足，请联系管理员"});

  auto l_task = std::make_shared<sd2::task>();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  l_json.get_to(*l_task);
  l_task->user_id_        = person_.person_.uuid_id_;
  l_task->ai_studio_id_   = person_.get_ai_studio_id();
  l_task->subproject_id_  = subproject_id_;
  l_task->file_extension_ = ".mp4";
  // data_request 必须有 content 字段，且 content 中可能 type 为 text 的字段
  auto& l_content         = l_task->data_request_.at("content");
  for (auto&& l_value : l_content)
    if (l_value.contains("type") && l_value.at("type").get<std::string>() == "text")
      l_task->text_prompt_ += l_value.at("text").get<std::string>() + "\n";

  auto l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);
  auto l_studio = l_sql.get_by_uuid<ai_studio>(l_task->ai_studio_id_);
  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
  auto l_ip  = co_await get_self_ip();
  auto l_req = add_ip_to_req(l_task->data_request_, l_ip);
#ifdef DOODLE_SEED2
  l_task->task_id_ = co_await l_client->run_task(l_req);  // 异步运行任务，不等待结果
#endif
  {
    auto l_add_tokens = add_remaining_tokens_for_person(l_sql, person_.person_.uuid_id_, -l_task->completion_tokens_);
    auto l_install    = orm::insert(l_sql).into<sd2::task>().values(*l_task);
    auto l_result_map = get_task_similarity_for_person(l_sql, *l_task);
    if (l_result_map.empty()) {
      co_await l_sql.run_sql(l_add_tokens, l_install);
    } else {
      auto l_install_similarities = orm::insert(l_sql).into<sd2::task_similarity>().set_range(l_result_map);
      co_await l_sql.run_sql(l_add_tokens, l_install, l_install_similarities);
    }
  }
  seedance2_task_run_manager::Get().run();
  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, put) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.status_ == sd2::task_status::queued, bad_request, "只有排队中的任务可以删除");
  auto l_studio = l_sql.get_by_uuid<ai_studio>(person_.get_ai_studio_id());
  auto l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);

  l_client->set_token(l_studio.app_secret_);
  l_client->set_logger(g_logger_ctrl().get_http());
  DOODLE_CHICK_HTTP(!l_task.task_id_.empty(), internal_server_error, "task id 为空, 无法查询");
  auto l_res = co_await l_client->query_task(l_task.task_id_);
  const sd2::task_status l_status{
      l_res.contains("status") ? l_res.at("status").get<sd2::task_status>() : sd2::task_status::failed
  };
  DOODLE_CHICK_HTTP(l_status == sd2::task_status::queued, bad_request, "只有排队中的任务可以删除");
#ifdef DOODLE_SEED2
  co_await l_client->cancel_task(l_task.task_id_);
#endif
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql        = get_sqlite_database();
  auto l_task       = std::make_shared<sd2::task>(l_sql.get_by_uuid<sd2::task>(id_));
  l_task->archived_ = true;
  co_await l_sql.update(l_task);
  co_return in_handle->make_msg(nlohmann::json{{"id", id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, get) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get) {
  // todo: 在 task 中, 添加 subproject 外键, 以方便查询
  auto l_sql = get_sqlite_database();
  using namespace orm;

  std::int32_t l_size   = 100;
  std::int32_t l_offset = 0;
  for (auto&& [key, value, has_value] : in_handle->url_.params()) {
    if (key == "size") l_size = std::stoi(value);
    if (key == "offset") l_offset = std::stoi(value);
  }

  auto l_query = select(l_sql)
                     .columns(object<sd2::task>())
                     .from<sd2::task>()
                     .join<sd2::subproject>(c(&sd2::task::subproject_id_) == c(&sd2::subproject::uuid_id_))
                     .order_by(&sd2::task::created_at_, false);
  if (!person_.is_manager())
    l_query.where(
        c(&sd2::subproject::uuid_id_)
            .in(select(l_sql)
                    .columns(&sd2::subproject_person_link::subproject_id_)
                    .from<sd2::subproject_person_link>()
                    .where(c(&sd2::subproject_person_link::person_id_) == person_.person_.uuid_id_)) &&
        !c(&sd2::task::archived_)
    );
  co_return in_handle->make_msg(nlohmann::json{} = l_query.limit(l_size).offset(l_offset)().to_vector());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_date, get) {
  auto l_sql = get_sqlite_database();
  person_.check_manager();
  using namespace orm;
  chrono::system_zoned_time l_date_start{chrono::current_zone(), chrono::sys_days{date_start_}};
  chrono::system_zoned_time l_date_end{
      chrono::current_zone(), chrono::sys_days{date_end_} + chrono::days{1} - chrono::seconds{1}
  };
  auto l_query = select(l_sql)
                     .columns(object<sd2::task>())
                     .from<sd2::task>()
                     .where(c(&sd2::task::created_at_) >= l_date_start && c(&sd2::task::created_at_) <= l_date_end)
                     .order_by(&sd2::task::created_at_, false);
  co_return in_handle->make_msg(nlohmann::json{} = l_query().to_vector());
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_thumbnail, get) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_thumbnail_file(id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "缩略图不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_pictures, get) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_pictures_file(id_, file_extension_.file_extension_.generic_string());
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_animation_waiting, get) {
  auto l_file = g_ctx().get<kitsu_ctx_t>().front_end_root_ / "seedance2" / "animation" / "waiting.mp4";
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "文件不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}

}  // namespace doodle::http::seedance2