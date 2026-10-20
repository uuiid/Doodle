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
#include <doodle_lib/http_client/ai_client_base.h>
#include <doodle_lib/http_client/apipy_image_client.h>
#include <doodle_lib/http_client/gemini_image_client.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_client/transfer_station_client.h>
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
#include <magic_enum/magic_enum.hpp>
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

// 从扩展名推断是否为视频, 其余按图片处理
void create_preview_picture(const FSys::path& in_file_path, const uuid& in_id) {
  auto l_ext            = in_file_path.extension().generic_string();
  auto l_is_video       = l_ext == ".mp4" || l_ext == ".mov" || l_ext == ".avi";
  auto l_file_picture   = g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(in_id, l_ext);
  auto l_file_thumbnail = g_ctx().get<kitsu_ctx_t>().get_sd2_thumbnail_file(in_id);
  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  {
    // 生成预览文件
    cv::Mat l_image{};
    if (l_is_video) {
      auto l_video = cv::VideoCapture{in_file_path.generic_string()};
      // 读取第一帧生成预览文件
      l_video >> l_image;
      if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});
    } else {
      l_image = cv::imread(in_file_path.generic_string());
      if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});
    }
    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));

    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }
  FSys::rename(in_file_path, l_file_picture);
}

class client_factory {
  struct client_pair {
    std::shared_ptr<seedance2_client> seedance2_{};
    std::shared_ptr<transfer_station_client> transfer_station_{};
    std::shared_ptr<apipy_image_client> apipy_image_{};
    std::shared_ptr<gemini_image_client> gemini_image_{};
  };
  std::map<uuid, client_pair> clients_;

 public:
  std::shared_ptr<ai_client_base> get_client(const sd2::task& in_task, const ai_studio& in_studio) {
    auto& l_pair = clients_[in_studio.uuid_id_];

    if (in_task.backend_ == sd2::task_backend::transfer_station) {
      if (!l_pair.transfer_station_) {
        l_pair.transfer_station_ = std::make_shared<transfer_station_client>(*core_set::get_set().ctx_ptr);
        l_pair.transfer_station_->set_token(in_studio.transfer_station_key_);
        l_pair.transfer_station_->set_logger(g_logger_ctrl().get_http());
        l_pair.transfer_station_->set_url_sign_secret(get_url_sign_secret());
      }
      return l_pair.transfer_station_;
    }

    if (in_task.backend_ == sd2::task_backend::apipy_image) {
      if (!l_pair.apipy_image_) {
        l_pair.apipy_image_ = std::make_shared<apipy_image_client>(*core_set::get_set().ctx_ptr);
        l_pair.apipy_image_->set_token(in_studio.apipy_key_);
        l_pair.apipy_image_->set_logger(g_logger_ctrl().get_http());
        // 参考图不走公网签名 URL: 本后端把参考图读成本地字节后作为文件负担上传, 上游看不到 URL。
        // 所以这里**不**调 set_url_sign_secret —— 与另外两个后端相反, 见 apipy_image_client.h 的
        // 「参考图 URL 的契约」。
      }
      return l_pair.apipy_image_;
    }

    if (in_task.backend_ == sd2::task_backend::gemini_image) {
      if (!l_pair.gemini_image_) {
        l_pair.gemini_image_ = std::make_shared<gemini_image_client>(*core_set::get_set().ctx_ptr);
        // 与 API易 同理: 参考图变成内联 base64 直接进请求体, 上游拿不到也访问不到本站 URL,
        // 公网签名只是白算一遍, 所以同样不设 url_sign_secret。
        l_pair.gemini_image_->set_token(in_studio.gemini_key_);
        l_pair.gemini_image_->set_logger(g_logger_ctrl().get_http());
      }
      return l_pair.gemini_image_;
    }

    // 默认 seedance2
    if (!l_pair.seedance2_) {
      l_pair.seedance2_ = std::make_shared<seedance2_client>(*core_set::get_set().ctx_ptr);
      l_pair.seedance2_->set_token(in_studio.seedance2_key_);
      l_pair.seedance2_->set_logger(g_logger_ctrl().get_http());
      l_pair.seedance2_->set_url_sign_secret(get_url_sign_secret());
    }
    return l_pair.seedance2_;
  }

  /// 公网素材 URL 的签名密钥(主服务器才有注册表密钥; 拿不到就不签名)
  static std::string get_url_sign_secret() {
    return g_ctx().contains<kitsu_ctx_t>() ? g_ctx().get<kitsu_ctx_t>().secret_ : std::string{};
  }
};

class seedance2_task_run_manager {
  struct task_info {
    explicit task_info(const sd2::task& in_task, const ai_studio& in_ai_studio)
        : task_(in_task), ai_studio_(in_ai_studio) {}
    sd2::task task_;
    ai_studio ai_studio_;
  };

  std::atomic_bool is_running_{false};

  std::vector<task_info> get_task() {
    auto l_sql = get_sqlite_database();
    using namespace orm;
    return select(l_sql)
        .columns(object<sd2::task>(), object<ai_studio>())
        .from<sd2::task>()
        .where(
            c(&sd2::task::status_) == sd2::task_status::preparing ||
            c(&sd2::task::status_) == sd2::task_status::queued || c(&sd2::task::status_) == sd2::task_status::running
        )
        .left_outer_join<ai_studio>(&ai_studio::uuid_id_, &sd2::task::ai_studio_id_)()
        .to_vector<task_info>();
  }

  /**
   * 跑一次任务周期并把结果落库。
   *
   * 流程本身(提交 / 查询 / 成功后下载)在 ai_client_base::run_task 里 —— 那是与后端无关的部分。
   * 这里只做三件调用方才有资格做的事:
   *   1. 选后端、建/取客户端(需要 ai_studio 里的 key, 基类拿不到);
   *   2. 按返回的状态落库(状态机、退补 token、回滚生成次数);
   *   3. 广播。
   *
   * 于是「提交」与「查询」不再各写一份落库逻辑(它们要写的字段本来也几乎一样),
   * 只有需要区别对待的分支才分叉。
   */
  boost::asio::awaitable<void> run_once(
      const sd2::task& in_task, const std::shared_ptr<ai_client_base>& in_client
  ) try {
    auto l_result = co_await in_client->run_task(in_task);
    auto l_sql    = get_sqlite_database();
    using namespace orm;
    sql_modify_statement_vector_t l_sql_modify_statements;
    auto l_update             = update(l_sql)
                                    .from<sd2::task>()
                                    .set(c(&sd2::task::data_response_) = l_result.data_response_)
                                    .where(c(&sd2::task::uuid_id_) == in_task.uuid_id_);
    sd2::task_status l_status = l_result.status_;

    switch (l_result.status_) {
      case sd2::task_status::succeeded: {
        // 两条路径都会到这里:
        //   * 同步任务(推理 / API易 图片): 提交那一次就直接成功;
        //   * 异步任务: 查询到 succeeded, 文件已由 run_task 下载落盘。
        const auto l_actual_tokens = l_result.total_tokens_ > 0 ? l_result.total_tokens_ : in_task.completion_tokens_;
        // 异步生成类的计费口径是 completion_tokens_(本站点数); 推理类是 total_tokens_(模型用量)。
        const auto l_charge        = l_result.completion_tokens_ > 0 ? l_result.completion_tokens_ : l_actual_tokens;
        l_update.set(
            c(&sd2::task::status_) = sd2::task_status::succeeded, c(&sd2::task::task_id_) = l_result.task_id_,
            c(&sd2::task::ended_at_) = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
            c(&sd2::task::completion_tokens_) = l_charge
        );
        // 结果文件(异步任务在这里才拿到, 同步任务在返回值里已经填好)
        if (!l_result.result_file_paths_.empty()) {
          auto l_adjusted_file = l_result.result_file_paths_[0];
          // 结果文件生成后, 从真实文件名取后缀
          sd2::ai_preview_file l_preview_file{};
          l_preview_file.extension_ = l_adjusted_file.extension().generic_string();
          l_sql_modify_statements.emplace_back(insert(l_sql).into<sd2::ai_preview_file>().values(l_preview_file));
          l_update.set(
              c(&sd2::task::file_extension_) = l_preview_file.extension_,
              c(&sd2::task::preview_file_)   = l_preview_file.uuid_id_
          );
          // 缩略图与落库顺序无关: 它读的是磁盘上的文件, 而文件已由 run_task 下载好。
          create_preview_picture(l_adjusted_file, l_preview_file.uuid_id_);
        }
        l_status = sd2::task_status::succeeded;
        // 预扣的是 default_consumed_tokens, 按实际的 l_charge 退补
        l_sql_modify_statements.emplace_back(
            add_remaining_tokens_for_person(l_sql, in_task.user_id_, in_task.completion_tokens_ - l_charge)
        );
        break;
      }
      case sd2::task_status::preparing:
      case sd2::task_status::queued:
      case sd2::task_status::running: {
        // 中间态: 记下 task_id 与状态(首次提交会带回来 task_id), 等下一次轮询。
        // 状态没变时不写状态字段, 省掉一次无谓的 update。
        if (!l_result.task_id_.empty()) l_update.set(c(&sd2::task::task_id_) = l_result.task_id_);
        if (in_task.status_ != l_result.status_) l_update.set(c(&sd2::task::status_) = l_result.status_);
        l_status = l_result.status_;
        break;
      }
      default: {
        // failed / cancelled / violation / expired —— 终态失败
        // 超时的失败可以重试: 退回 preparing, 由下一轮重新提交
        if (l_result.is_timeout_ && in_task.retry_count_ < 100) {
          l_update.set(c(&sd2::task::retry_count_) = c(&sd2::task::retry_count_) + 1);
          l_update.set(c(&sd2::task::status_) = sd2::task_status::preparing);
          l_status = sd2::task_status::preparing;
          break;
        }
        l_update.set(
            c(&sd2::task::status_)   = l_result.status_,
            c(&sd2::task::ended_at_) = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
            c(&sd2::task::completion_tokens_) = l_result.completion_tokens_
        );
        l_status = l_result.status_;
        // 失败或者其他状态，返还 token
        l_sql_modify_statements.emplace_back(add_remaining_tokens_for_person(
            l_sql, in_task.user_id_, in_task.completion_tokens_ - l_result.completion_tokens_
        ));
        // 失败时回滚生成次数
        l_sql_modify_statements.emplace_back(
            update(l_sql)
                .from<sd2::ai_generate_entity>()
                .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) - 1)
                .where(c(&sd2::ai_generate_entity::uuid_id_) == in_task.ai_generate_entity_id_)
        );
        break;
      }
    }
    l_sql_modify_statements.push_back(l_update);
    co_await l_sql.run_sql(std::move(l_sql_modify_statements));
    socket_io::broadcast(
        socket_io::seedance2_task_update_broadcast_t{.task_id_ = in_task.uuid_id_, .status_ = l_status}
    );
  } catch (...) {
    auto l_err_str = boost::current_exception_diagnostic_information();
    SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), "任务 {} 执行失败: {}", in_task.uuid_id_, l_err_str);
  }

  boost::asio::awaitable<void> async_run() {
    boost::scope::scope_exit on_exit{[this]() { is_running_ = false; }};
    boost::asio::steady_timer l_timer{g_io_context()};
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      auto l_tasks = get_task();
      if (l_tasks.empty()) co_return;
      client_factory l_client_factory{};
      for (auto&& l_task_info : l_tasks) {
        // 选后端、建客户端只能在这里做: 它需要 ai_studio 里的 key, 而基类只拿得到 task。
        // 之后的一切(提交还是查询、要不要下载)由 run_task 自己按 task.status_ 判断。
        auto l_client = l_client_factory.get_client(l_task_info.task_, l_task_info.ai_studio_);
        co_await run_once(l_task_info.task_, l_client);
      }

      l_timer.expires_after(5s);
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
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

// ─── 推理入口(声明在 inference_task.h) ──────────────────────────────────────
// 这两个函数**必须在匿名命名空间之外**：写在 namespace { 里会变成内部链接的另一组函数,
// 编译链接都不报错, 但 shared library 里根本不导出它们 —— 直到有库外调用方
// (接缝 / 单测) 引用时才以 LNK2019 暴露出来。位置固定在匿名命名空间之后。

std::shared_ptr<ai_client_base> make_inference_client(const ai_studio& in_studio, sd2::task_backend in_backend) {
  // 没有 ssl context 就没有客户端。主服务器的初始化会建好它（launch/kitsu_supplement.cpp
  // 里的 ctx_ptr），但 worker / 单测宿主没有。原来的写法直接 `*ctx_ptr` —— 空指针解引用退化成一个
  // 空引用, 崩在 SSL_new 内部的 0x8 上, 报错完全没有指向性。这里换成明确的异常。
  auto l_ctx = core_set::get_set().ctx_ptr;
  DOODLE_CHICK(l_ctx != nullptr, "ssl context 未初始化(见 core_set::ctx_ptr), 无法创建推理客户端");

  std::shared_ptr<ai_client_base> l_client{};
  if (in_backend == sd2::task_backend::transfer_station)
    l_client = std::make_shared<transfer_station_client>(*l_ctx);
  else if (in_backend == sd2::task_backend::apipy_image)
    l_client = std::make_shared<apipy_image_client>(*l_ctx);
  else if (in_backend == sd2::task_backend::gemini_image)
    l_client = std::make_shared<gemini_image_client>(*l_ctx);
  else
    l_client = std::make_shared<seedance2_client>(*l_ctx);

  l_client->set_token([&]() {
    switch (in_backend) {
      case sd2::task_backend::transfer_station:
        return in_studio.transfer_station_key_;
      case sd2::task_backend::apipy_image:
        return in_studio.apipy_key_;
      case sd2::task_backend::gemini_image:
        return in_studio.gemini_key_;
      default:
        return in_studio.seedance2_key_;
    }
  }());
  l_client->set_logger(g_logger_ctrl().get_http());
  l_client->set_url_sign_secret(client_factory::get_url_sign_secret());
  return l_client;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> run_inference_task(
    const sd2::task& in_task, sqlite_database& in_sql, const std::shared_ptr<ai_client_base>& in_client
) {
  auto l_result         = co_await in_client->run_task(in_task);

  const bool l_ok       = l_result.status_ == sd2::task_status::succeeded;
  auto l_task           = in_task;
  l_task.status_        = l_ok ? sd2::task_status::succeeded : sd2::task_status::failed;
  l_task.task_id_       = l_result.task_id_;
  l_task.ended_at_      = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  l_task.data_response_ = l_result.data_response_;
  // 推理按 total_tokens 计费(文本+多图的**输入**才是成本大头); 拿不到 usage 就退回调用方给的预扣值。
  // 失败不计费(completion_tokens_ 记 0), 但原始响应仍留在 data_response_ 里便于对账。
  l_task.completion_tokens_ =
      l_ok ? (l_result.total_tokens_ > 0 ? l_result.total_tokens_ : in_task.completion_tokens_) : std::int64_t{0};

  {
    // 一个事务: 插入(终态) + 扣费。因为插入就是终态, 后台管理器(只捞
    // preparing/queued/running)永远看不到这条任务, 所以不存在重复提交。
    using namespace orm;
    sql_modify_statement_vector_t l_sqls;
    l_sqls.emplace_back(insert(in_sql).into<sd2::task>().values(l_task));
    if (l_task.completion_tokens_ > 0)
      l_sqls.emplace_back(add_remaining_tokens_for_person(in_sql, l_task.user_id_, -l_task.completion_tokens_));
    co_await in_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "推理任务 {} 结束: status {} tokens {} -> {}", l_task.uuid_id_,
      magic_enum::enum_name(l_task.status_), l_result.total_tokens_, l_task.completion_tokens_
  );
  socket_io::broadcast(
      socket_io::seedance2_task_update_broadcast_t{.task_id_ = l_task.uuid_id_, .status_ = l_task.status_}
  );
  co_return l_result;
}

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

seedance2_subproject_task::seedance2_subproject_task() { seedance2_task_run_manager::Get().run(); }
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task, post) {
  person_.check_subproject_access(subproject_id_);

  if (get_remaining_tokens_for_person(person_.person_.uuid_id_) - doodle_config::g_max_task_completion_tokens <= 0)
    throw_exception(doodle_error{"当周可用token数量不足，请联系管理员"});

  auto l_task = std::make_shared<sd2::task>();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  l_json.get_to(*l_task);
  // 推理任务(task_type::inference)不走这条前端提交路径: 它由服务端接缝同步执行并直接落终态
  // (见 run_inference_task), 不需要 ai_generate_entity / 分辨率 / 当周配额预扣。
  // 不在这里明确拒绝的话, 下面那句「缺少分辨率」会给出误导性的错误。
  DOODLE_CHICK_HTTP(
      l_task->type_ != sd2::task_type::inference, bad_request, "推理任务由服务端执行, 不能通过子项目任务接口提交"
  );
  l_task->user_id_       = person_.person_.uuid_id_;
  l_task->ai_studio_id_  = person_.get_ai_studio_id();
  l_task->subproject_id_ = subproject_id_;
  // file_extension_ 不再写死, 改为任务成功后按真实结果文件设置

  // 用对应后端的客户端解析请求, 提取模型 / 分辨率 / 提示词
  auto l_studio          = l_sql.get_by_uuid<ai_studio>(l_task->ai_studio_id_);
  client_factory l_client_factory{};
  auto l_client              = l_client_factory.get_client(*l_task, l_studio);
  auto l_info                = l_client->collect_request_info(l_task->data_request_);
  l_task->text_prompt_       = std::move(l_info.text_prompt_);
  l_task->completion_tokens_ = l_client->default_consumed_tokens(l_task->type_);

  DOODLE_CHICK_HTTP(!l_info.model_.empty(), bad_request, "缺少模型名称");
  DOODLE_CHICK_HTTP(!l_info.resolution_.empty(), bad_request, "缺少分辨率");

  {
    using namespace orm;
    auto l_entity = l_sql.get_by_uuid<sd2::ai_generate_entity>(l_task->ai_generate_entity_id_);
    auto l_result = select(l_sql)
                        .columns(count(&sd2::ai_episode_model_resolution_limit::id_))
                        .from<sd2::ai_episode_model_resolution_limit>()
                        .where(
                            c(&sd2::ai_episode_model_resolution_limit::ai_episode_id_) == l_entity.ai_episode_id_ &&
                            c(&sd2::ai_episode_model_resolution_limit::model_name_) == l_info.model_ &&
                            c(&sd2::ai_episode_model_resolution_limit::resolution_) == l_info.resolution_
                        )  //
                    ()
                        .to_single();
    DOODLE_CHICK_HTTP(l_result == 1, unauthorized, "模型 {} 或者分辨率 {} 未被授权", l_info.model_, l_info.resolution_);
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
      l_task.status_ == sd2::task_status::preparing || l_task.status_ == sd2::task_status::queued ||
          (l_task.status_ == sd2::task_status::running && l_task.backend_ == sd2::task_backend::seedance2),
      bad_request, "只有准备中或排队中,运行中的任务可以取消"
  );

  // preparing 状态尚未提交到外部, 直接置为 cancelled 并归还 token
  if (l_task.status_ != sd2::task_status::preparing) {
    auto l_studio = l_sql.get_by_uuid<ai_studio>(person_.get_ai_studio_id());
    client_factory l_client_factory{};
    auto l_client = l_client_factory.get_client(l_task, l_studio);

    DOODLE_CHICK_HTTP(!l_task.task_id_.empty(), internal_server_error, "task id 为空, 无法查询");
    auto l_res = co_await l_client->query_task(l_task);
    DOODLE_CHICK_HTTP(
        (l_res.status_ == sd2::task_status::queued || l_res.status_ == sd2::task_status::running) &&
            l_task.backend_ == sd2::task_backend::seedance2,
        bad_request, "只有排队中和运行中的任务可以取消"
    );
#ifdef DOODLE_SEED2
    co_await l_client->cancel_task(l_task.task_id_);
#endif
  }

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<sd2::task>()
                          .set(c(&sd2::task::status_) = sd2::task_status::cancelled)
                          .set(
                              c(&sd2::task::ended_at_) =
                                  chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()},
                              c(&sd2::task::completion_tokens_) = 0
                          )
                          .where(c(&sd2::task::uuid_id_) == l_task.uuid_id_));
  l_sqls.emplace_back(add_remaining_tokens_for_person(l_sql, l_task.user_id_, l_task.completion_tokens_));
  l_sqls.emplace_back(
      update(l_sql)
          .from<sd2::ai_generate_entity>()
          .set(c(&sd2::ai_generate_entity::generate_count_) = c(&sd2::ai_generate_entity::generate_count_) - 1)
          .where(c(&sd2::ai_generate_entity::uuid_id_) == l_task.ai_generate_entity_id_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));
  l_task.status_ = sd2::task_status::cancelled;
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_task_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<sd2::task>()
                          .set(c(&sd2::task::archived_) = true)
                          .where(c(&sd2::task::uuid_id_) == l_task.uuid_id_));
  co_await l_sql.run_sql(std::move(l_sqls));
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
  std::optional<sd2::task_status> l_status{};
  std::optional<sd2::task_type> l_task_type{};
  uuid l_project{};
  uuid l_user_id{};
  for (auto&& [key, value, has_value] : in_handle->url_.params()) {
    if (key == "size") l_size = std::stoi(value);
    if (key == "offset") l_offset = std::stoi(value);
    if (key == "status") l_status = magic_enum::enum_cast<sd2::task_status>(value).value();
    if (key == "task_type") l_task_type = magic_enum::enum_cast<sd2::task_type>(value).value();
    if (key == "project") l_project = from_uuid_str(value);
    if (key == "user_id") l_user_id = from_uuid_str(value);
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
  if (l_task_type) l_query.where(c(&sd2::task::type_) == *l_task_type);
  if (!l_project.is_nil()) l_query.where(c(&sd2::subproject::project_id_) == l_project);
  if (!l_user_id.is_nil()) l_query.where(c(&sd2::task::user_id_) == l_user_id);
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