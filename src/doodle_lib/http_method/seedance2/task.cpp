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
#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core/core_set.h"
#include "core/http_client_core.h"
#include "http_method/kitsu.h"
#include "reg.h"
#include "sqlite_orm/orm/column_operations.h"
#include "sqlite_orm/orm/count.h"
#include "sqlite_orm/orm/fwd.h"
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
#include <string_view>
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
        .columns(object<sd2::task>(), &ai_studio::transfer_station_key_)
        .from<sd2::task>()
        .where(
            c(&sd2::task::status_) == sd2::task_status::preparing ||
            c(&sd2::task::status_) == sd2::task_status::queued || c(&sd2::task::status_) == sd2::task_status::running
        )
        .left_outer_join<ai_studio>(&ai_studio::uuid_id_, &sd2::task::ai_studio_id_)()
        .to_vector<task_info>();
  }

  boost::asio::awaitable<void> submit_task(
      const sd2::task& in_task, const std::shared_ptr<seedance2_client>& in_client
  ) try {
    sd2::task_status l_status{};
    auto l_result = co_await in_client->run_task(in_task.data_request_);
    auto l_sql    = get_sqlite_database();
    using namespace orm;
    sql_modify_statement_vector_t l_sql_modify_statements;
    auto l_update = update(l_sql)
                        .from<sd2::task>()
                        .set(c(&sd2::task::data_response_) = l_result.data_response_)
                        .where(c(&sd2::task::uuid_id_) == in_task.uuid_id_);
    l_sql_modify_statements.push_back(l_update);
    if (l_result.status_ == sd2::task_status::queued) {
      l_update.set(c(&sd2::task::task_id_) = l_result.task_id_);
      l_update.set(c(&sd2::task::status_) = sd2::task_status::queued);
      l_status = sd2::task_status::queued;
    } else if (l_result.status_ == sd2::task_status::failed && l_result.is_timeout_ && in_task.retry_count_ < 100) {
      l_update.set(c(&sd2::task::retry_count_) = c(&sd2::task::retry_count_) + 1);
      l_status = sd2::task_status::preparing;
    } else {
      l_update.set(c(&sd2::task::status_) = sd2::task_status::failed);
      l_status = sd2::task_status::failed;
      l_update.set(
          c(&sd2::task::ended_at_) = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
          c(&sd2::task::completion_tokens_) = 0
      );
      // 任务失败或者其他状态，返还 token
      l_sql_modify_statements.emplace_back(
          add_remaining_tokens_for_person(l_sql, in_task.user_id_, in_task.completion_tokens_)
      );
      // 失败时回滚生成次数
      l_sql_modify_statements.emplace_back(
          update(l_sql)
              .from<sd2::ai_generate_entity>()
              .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) - 1)
              .where(c(&sd2::ai_generate_entity::uuid_id_) == in_task.ai_generate_entity_id_)
      );
    }
    co_await l_sql.run_sql(l_sql_modify_statements);
    socket_io::broadcast(
        socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_status}
    );
  } catch (...) {
    auto l_err_str = boost::current_exception_diagnostic_information();
    SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), "提交任务 {} 失败: {}", in_task.uuid_id_, l_err_str);
  }

  boost::asio::awaitable<void> async_run() {
    boost::scope::scope_exit on_exit{[this]() { is_running_ = false; }};
    boost::asio::steady_timer l_timer{g_io_context()};
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      std::map<std::string, std::shared_ptr<seedance2_client>> l_client_map;
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
        if (l_task_info.task_.status_ == sd2::task_status::preparing) {
          co_await submit_task(l_task_info.task_, l_client);
        } else {
          co_await query_task_and_down(l_task_info.task_, l_client);
        }
      }

      l_timer.expires_after(5s);
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
  }

  boost::asio::awaitable<void> query_task_and_down(
      const sd2::task& in_task, const std::shared_ptr<seedance2_client>& in_client
  ) try {
    ai_client_base::query_task_result_t l_result;
    try {
      l_result = co_await in_client->query_task(in_task.task_id_);
    } catch (const doodle_error& in_err) {
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_main_error(), "查询任务 {} 失败, 错误: {}", in_task.uuid_id_, in_err.what()
      );
      l_result.status_        = sd2::task_status::failed;
      l_result.data_response_ = in_err.what();
    }
    auto l_sql = get_sqlite_database();
    using namespace orm;
    sql_modify_statement_vector_t l_sqls;
    uuid l_preview_file_id{};

    switch (l_result.status_) {
      case sd2::task_status::preparing:
      case sd2::task_status::queued:
        co_return;
      case sd2::task_status::running: {
        if (in_task.status_ != l_result.status_) {
          co_await l_sql.run_sql(update(l_sql)
                                     .from<sd2::task>()
                                     .set(c(&sd2::task::status_) = l_result.status_)
                                     .set(c(&sd2::task::data_response_) = l_result.data_response_)
                                     .where(c(&sd2::task::uuid_id_) == in_task.uuid_id_));
        }
        socket_io::broadcast(
            socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_result.status_}
        );
        co_return;
      }
      case sd2::task_status::succeeded: {
        // 重新查询一次以获取 result_files_, 并下载
        if (!l_result.result_files_.empty()) {
          co_await l_result.download();
        }
        if (!l_result.result_file_paths_.empty()) {
          auto l_adjusted_file = l_result.result_file_paths_[0];
          sd2::ai_preview_file l_preview_file{};
          l_preview_file.extension_ = l_adjusted_file.extension().generic_string();
          l_sqls.emplace_back(insert(l_sql).into<sd2::ai_preview_file>().values(l_preview_file));
          l_preview_file_id = l_preview_file.uuid_id_;
          video_create_picture(l_adjusted_file, l_preview_file.uuid_id_);
        }
        break;
      }
      case sd2::task_status::cancelled:
      case sd2::task_status::failed:
      case sd2::task_status::expired:
        // 以上状态不扣费
        l_result.completion_tokens_ = 0;
        break;
    }

    l_sqls.emplace_back(update(l_sql)
                            .from<sd2::task>()
                            .set(c(&sd2::task::status_) = l_result.status_)
                            .set(
                                c(&sd2::task::ended_at_) =
                                    chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}
                            )
                            .set(c(&sd2::task::data_response_) = l_result.data_response_)
                            .set(c(&sd2::task::completion_tokens_) = l_result.completion_tokens_)
                            .set(c(&sd2::task::preview_file_) = l_preview_file_id)  // 非成功清零
                            .where(c(&sd2::task::uuid_id_) == in_task.uuid_id_));
    if (l_result.status_ == sd2::task_status::succeeded) {
      // 为负数时, 如果任务成功，说明实际消耗的 token 比预估的少，返还差值
      l_sqls.emplace_back(add_remaining_tokens_for_person(
          l_sql, in_task.user_id_, in_task.completion_tokens_ - l_result.completion_tokens_
      ));
    } else {
      // 任务失败或者其他状态，返还 token
      l_sqls.emplace_back(add_remaining_tokens_for_person(l_sql, in_task.user_id_, in_task.completion_tokens_));
      // 失败时回滚生成次数
      l_sqls.emplace_back(
          update(l_sql)
              .from<sd2::ai_generate_entity>()
              .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) - 1)
              .where(c(&sd2::ai_generate_entity::uuid_id_) == in_task.ai_generate_entity_id_)
      );
    }
    co_await l_sql.run_sql(std::move(l_sqls));
    socket_io::broadcast(
        socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_result.status_}
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

  bool is_running() const { return is_running_; }

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

seedance2_subproject_task::seedance2_subproject_task() {
#ifdef NDEBUG
  seedance2_task_run_manager::Get().run();
#endif
}
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
  // 获取模型和分辨率字段(为必填项, 不检测存在)
  std::string l_model      = l_task->data_request_.at("model").get<std::string>();
  std::string l_resolution = l_task->data_request_.at("resolution").get<std::string>();

  {
    using namespace orm;
    auto l_entity = l_sql.get_by_uuid<sd2::ai_generate_entity>(l_task->ai_generate_entity_id_);
    auto l_result = select(l_sql)
                        .columns(count(&sd2::ai_episode_model_resolution_limit::id_))
                        .from<sd2::ai_episode_model_resolution_limit>()
                        .where(
                            c(&sd2::ai_episode_model_resolution_limit::ai_episode_id_) == l_entity.ai_episode_id_ &&
                            c(&sd2::ai_episode_model_resolution_limit::model_name_) == l_model &&
                            c(&sd2::ai_episode_model_resolution_limit::resolution_) == l_resolution
                        )  //
                    ()
                        .to_single();
    DOODLE_CHICK_HTTP(l_result == 1, unauthorized, "模型 {} 或者分辨率 {} 未被授权", l_model, l_resolution);
  }
  {
    using namespace orm;
    auto l_entity  = l_sql.get_by_uuid<sd2::ai_generate_entity>(l_task->ai_generate_entity_id_);
    auto l_episode = l_sql.get_by_uuid<sd2::ai_episode>(l_entity.ai_episode_id_);
    DOODLE_CHICK_HTTP(
        l_entity.generate_count_ < l_episode.limit_count_ || l_episode.limit_count_ == 0, bad_request,
        "生成次数已达上限 {} 次", l_episode.limit_count_
    );
  }
  {
    using namespace orm;
    sql_modify_statement_vector_t l_sqls;
    l_sqls.emplace_back(insert(l_sql).into<sd2::task>().values(*l_task));
    auto l_result_map = get_task_similarity_for_person(l_sql, *l_task);
    l_sqls.emplace_back(add_remaining_tokens_for_person(l_sql, person_.person_.uuid_id_, -l_task->completion_tokens_));
    l_sqls.emplace_back(
        update(l_sql)
            .from<sd2::ai_generate_entity>()
            .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) + 1)
            .where(c(&sd2::ai_generate_entity::uuid_id_) == l_task->ai_generate_entity_id_)
    );
    l_sqls.emplace_back(insert(l_sql).into<sd2::task_similarity>().set_range(l_result_map));
    co_await l_sql.run_sql(std::move(l_sqls));
  }
  seedance2_task_run_manager::Get().run();
  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, put) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(
      l_task.status_ == sd2::task_status::preparing || l_task.status_ == sd2::task_status::queued, bad_request,
      "只有准备中或排队中的任务可以取消"
  );

  // preparing 状态尚未提交到外部, 直接置为 cancelled 并归还 token
  if (l_task.status_ != sd2::task_status::preparing) {
    auto l_studio = l_sql.get_by_uuid<ai_studio>(person_.get_ai_studio_id());
    auto l_client = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);

    l_client->set_token(l_studio.transfer_station_key_);
    l_client->set_logger(g_logger_ctrl().get_http());
    DOODLE_CHICK_HTTP(!l_task.task_id_.empty(), internal_server_error, "task id 为空, 无法查询");
    auto l_res = co_await l_client->query_task(l_task.task_id_);
    DOODLE_CHICK_HTTP(l_res.status_ == sd2::task_status::queued, bad_request, "只有排队中的任务可以取消");
#ifdef DOODLE_SEED2
    co_await l_client->cancel_task(l_task.task_id_);
#endif
  }

  using namespace orm;
  co_await l_sql.run_sql(
      update(l_sql)
          .from<sd2::task>()
          .set(c(&sd2::task::status_) = sd2::task_status::cancelled)
          .set(
              c(&sd2::task::ended_at_) = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
              c(&sd2::task::completion_tokens_) = 0
          )
          .where(c(&sd2::task::uuid_id_) == l_task.uuid_id_),
      add_remaining_tokens_for_person(l_sql, l_task.user_id_, l_task.completion_tokens_),
      update(l_sql)
          .from<sd2::ai_generate_entity>()
          .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) - 1)
          .where(c(&sd2::ai_generate_entity::uuid_id_) == l_task.ai_generate_entity_id_)
  );
  l_task.status_ = sd2::task_status::cancelled;
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  using namespace orm;
  co_await l_sql.run_sql(update(l_sql)
                             .from<sd2::task>()
                             .set(c(&sd2::task::archived_) = true)
                             .where(c(&sd2::task::uuid_id_) == l_task.uuid_id_));
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
  std::optional<sd2::task_status> l_status;
  for (auto&& [key, value, has_value] : in_handle->url_.params()) {
    if (key == "size") l_size = std::stoi(value);
    if (key == "offset") l_offset = std::stoi(value);
    if (key == "status") l_status = nlohmann::json(value).get<sd2::task_status>();
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
  if (l_status) l_query.where(c(&sd2::task::status_) == *l_status);
  co_return in_handle->make_msg(nlohmann::json{} = l_query.limit(l_size).offset(l_offset)().to_vector());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_run, get) {
  co_return in_handle->make_msg(nlohmann::json{{"running", seedance2_task_run_manager::Get().is_running()}});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_run, post) {
  seedance2_task_run_manager::Get().run();
  co_return in_handle->make_msg(nlohmann::json{{"running", seedance2_task_run_manager::Get().is_running()}});
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
  auto l_file = g_ctx().get<kitsu_ctx_t>().get_seedance2_waiting();
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "文件不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}

}  // namespace doodle::http::seedance2