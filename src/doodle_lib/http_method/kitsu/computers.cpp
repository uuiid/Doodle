#include "computers.h"

#include "doodle_core/metadata/computer.h"

#include "doodle_lib/core/app_base.h"
#include "doodle_lib/core/http/http_function.h"
#include "doodle_lib_fwd.h"
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/lockfree/spsc_value.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core/global_function.h"
#include <atomic>
#include <chrono>
#include <fmt/ranges.h>
#include <functional>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>

namespace doodle::http {
namespace {
std::optional<computer> get_entity_computer_by_hardware_id(const uuid& in_hardware_id) {
  auto l_sql = get_sqlite_database();
  using namespace orm;
  return select(l_sql)
      .columns(object<computer>())
      .from<computer>()
      .where(c(&computer::hardware_id_) == in_hardware_id)
      .limit(1)()
      .to_optional();
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers, get) {
  person_.check_not_outsourcer();
  auto l_sql       = get_sqlite_database();
  auto l_computers = l_sql.get_all<computer>();
  co_return in_handle->make_msg(nlohmann::json{} = l_computers);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql)
                      .from<computer>()
                      .set_from_ref<computer>(l_json)
                      .set(
                          c(&computer::last_heartbeat_time_) =
                              chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}
                      )
                      .where(c(&computer::uuid_id_) == computer_id_);
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = computer_id_});
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<computer>().where(c(&computer::uuid_id_) == computer_id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  socket_io::broadcast(socket_io::computer_delete_broadcast_t{.computer_id_ = computer_id_});
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

class data_computers_socket_io_impl : public std::enable_shared_from_this<data_computers_socket_io_impl> {
  std::shared_ptr<boost::beast::websocket::stream<http::tcp_stream_type>> web_stream_;
  // computer 各属性的原子快照, 取代原先的 std::shared_ptr<computer> computer_。
  // 写方恒为本对象的 strand_(见 run()), 读方可来自任意执行器; 发布用 release, 读取用 acquire。
  // 标量属性直接存 std::atomic<T>, 非平凡可复制的属性用 atomic<shared_ptr<T>> 发布不可变副本。
  std::atomic<uuid> uuid_id_{};
  std::atomic<uuid> hardware_id_{};
  std::atomic<std::shared_ptr<std::set<server_task_info_type>>> allowed_task_types_{};

  // 分配器视角的状态(get_computer_status / set_computer_status(computer_status) 读写)。
  // 由 init() 按注册消息首次发布, 之后每次 set_computer_status 刷新;
  // 连接断开时置 offline(见 async_run 的读循环 scope_exit)。
  std::atomic<computer_status> last_status_{computer_status::offline};

  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>> message_queue_;
  std::atomic<bool> writing_{false}, should_close_{false};
  boost::lockfree::spsc_value<boost::beast::websocket::ping_data> ping_message_;

  // 第一步, 等待计算机发送自身信息, 第二步, 将计算机信息保存到数据库
  boost::asio::awaitable<void> init() {
    boost::beast::flat_buffer l_buffer{};

    co_await web_stream_->async_read(l_buffer);
    auto l_json =
        nlohmann::json::parse(boost::asio::buffers_begin(l_buffer.data()), boost::asio::buffers_end(l_buffer.data()));
    auto l_computer_json       = l_json.get<computer>();
    // ORM 的 insert/update 需要一个完整的 computer 对象, 这里保留一个局部副本(不再是成员)
    auto l_row                 = l_computer_json;
    l_row.last_heartbeat_time_ = std::chrono::system_clock::now();
    auto l_sql                 = get_sqlite_database();
    if (auto l_db_computer = get_entity_computer_by_hardware_id(l_row.hardware_id_); l_db_computer.has_value()) {
      l_row         = l_db_computer.value();
      l_row.status_ = l_computer_json.status_;
      using namespace orm;
      sql_modify_statement_vector_t l_sqls{};
      l_sqls.emplace_back(update(l_sql)
                              .from<computer>()
                              .set(c(&computer::status_) = l_computer_json.status_)
                              .where(c(&computer::uuid_id_) == l_row.uuid_id_));
      co_await l_sql.run_sql(std::move(l_sqls));
    } else {
      using namespace orm;
      sql_modify_statement_vector_t l_sqls{};
      l_sqls.emplace_back(insert(l_sql).into<computer>().values(l_row));
      co_await l_sql.run_sql(std::move(l_sqls));
    }
    // 逐属性发布快照, 之后每次 set_computer_status 都会刷新
    uuid_id_.store(l_row.uuid_id_, std::memory_order_release);
    hardware_id_.store(l_row.hardware_id_, std::memory_order_release);
    allowed_task_types_.store(
        std::make_shared<std::set<server_task_info_type>>(l_row.allowed_task_types_), std::memory_order_release
    );
    // 注册消息本身就是一次状态上报, 必须在这里发布: 客户端连接时只发这一条消息,
    // 之后没有周期性心跳(ping 是 websocket 控制帧, 不会走到 set_computer_status)。
    // 漏掉这一行的话 last_status_ 会一直停在 offline —— 库里 status_ 是 online(UI 显示在线),
    // 但 run_next_task 的在线判据永远不成立, 机器一个任务都拿不到。
    last_status_.store(l_computer_json.status_, std::memory_order_release);
  }
  void write_msg(const std::string& in_msg) { message_queue_.push(in_msg); }
  friend class computers_assign_task;

  boost::asio::awaitable<void> async_run() {
    co_await init();
    co_await computers_assign_task::get_instance().register_computer(shared_from_this());
    begin_ping();
    boost::scope::scope_exit l_{[this, sh = shared_from_this()]() {
      try {
        auto l_uuid = uuid_id_.load(std::memory_order_acquire);
        boost::asio::co_spawn(
            g_io_context(),
            [l_uuid]() -> boost::asio::awaitable<void> {
              auto l_sql = get_sqlite_database();
              using namespace orm;
              auto l_now   = chrono::system_clock::now();
              auto l_zoned = chrono::system_zoned_time{chrono::current_zone(), l_now};
              sql_modify_statement_vector_t l_sqls{};
              l_sqls.emplace_back(update(l_sql)
                                      .from<computer>()
                                      .set(c(&computer::status_) = computer_status::offline)
                                      .set(c(&computer::last_heartbeat_time_) = l_zoned)
                                      .where(c(&computer::uuid_id_) == l_uuid));
              // 断开时把仍挂在这台机器名下的 running 任务解绑(状态保留 running 供人工确认),
              // 否则该机器重连后会被这条孤儿任务一直挡住, 再也拿不到任务
              l_sqls.emplace_back(update(l_sql)
                                      .from<server_task_info>()
                                      .set(c(&server_task_info::run_computer_id_) = uuid{})
                                      .where(
                                          c(&server_task_info::run_computer_id_) == l_uuid &&
                                          c(&server_task_info::status_) == server_task_info_status::running
                                      ));
              co_await l_sql.run_sql(std::move(l_sqls));
            },
            boost::asio::detached
        );
        socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = l_uuid});
      } catch (...) {
        SPDLOG_LOGGER_ERROR(
            g_logger_ctrl().get_http(), "清理计算机 {} 状态时发生异常: {}", get_computer_id(),
            boost::current_exception_diagnostic_information()
        );
      }
    }};
    try {
      boost::scope::scope_exit l_{[this, sh = shared_from_this()]() {
        should_close_ = true;
        // 读循环一结束就标记离线: 本对象还会被 ping 协程持有最多一个 ping 周期,
        // 期间 run_next_task 不能把它当成在线机器派任务(派了也发不出去)
        last_status_.store(computer_status::offline, std::memory_order_release);
      }};
      while ((co_await boost::asio::this_coro::cancellation_state).cancelled() ==
             boost::asio::cancellation_type::none) {
        // boost::beast::flat_buffer l_buffer{};
        std::string l_body{};
        auto l_buffer = boost::asio::dynamic_buffer(l_body);
        if (!web_stream_) co_return;
        co_await web_stream_->async_read(l_buffer);
        auto l_json = nlohmann::json::parse(
            boost::asio::buffers_begin(l_buffer.data()), boost::asio::buffers_end(l_buffer.data())
        );
        auto l_computer = l_json.get<computer>();
        co_await set_computer_status(l_computer);
      }
    } catch (...) {
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_http(), "计算机 {} 连接发生错误: {}", get_computer_id(),
          boost::current_exception_diagnostic_information()
      );
    }
    co_await web_stream_->async_close(boost::beast::websocket::close_code::normal, boost::asio::use_awaitable);
  }

  boost::asio::awaitable<void> write_websocket() {
    if (writing_ || should_close_) co_return;
    writing_ = true;
    boost::scope::scope_exit l_{[this, sh = shared_from_this()]() { writing_ = false; }};

    if (ping_message_.read(boost::lockfree::uses_optional))
      co_await web_stream_->async_ping(boost::beast::websocket::ping_data{});

    while (!message_queue_.empty()) {
      std::string l_msg{};
      message_queue_.pop(l_msg);
      if (!web_stream_) break;
      co_await web_stream_->async_write(boost::asio::buffer(l_msg), boost::asio::use_awaitable);
    }
  }
  boost::asio::awaitable<void> set_computer_status(std::reference_wrapper<computer> in_computer) {
    auto l_sql    = get_sqlite_database();
    auto l_uuid   = uuid_id_.load(std::memory_order_acquire);
    // name 从库中刷新(保持原有行为), 其余属性取自本次上报
    auto l_status = in_computer.get().status_;
    auto l_now    = chrono::system_clock::now();
    auto l_zoned  = chrono::system_zoned_time{chrono::current_zone(), l_now};
    // 本协程运行在 strand_ 上; 发布后任务分配 strand 才会看到新值
    allowed_task_types_.store(
        std::make_shared<std::set<server_task_info_type>>(in_computer.get().allowed_task_types_),
        std::memory_order_release
    );
    last_status_ = l_status;
    using namespace orm;
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(update(l_sql)
                            .from<computer>()
                            .set(c(&computer::status_) = l_status)
                            .set(c(&computer::last_heartbeat_time_) = l_zoned)
                            .where(c(&computer::uuid_id_) == l_uuid));
    co_await l_sql.run_sql(std::move(l_sqls));
    if (l_status == computer_status::online) co_await computers_assign_task::get_instance().run_next_task();

    socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = l_uuid});
  }
  void begin_ping() {
    boost::asio::co_spawn(
        strand_, async_ping_loop(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }
  boost::asio::awaitable<void> async_ping_loop() {
    try {
      boost::asio::steady_timer timer{co_await boost::asio::this_coro::executor};
      while ((co_await boost::asio::this_coro::cancellation_state).cancelled() ==
                 boost::asio::cancellation_type::none &&
             web_stream_ && web_stream_->is_open()) {
        timer.expires_after(std::chrono::seconds(30));
        co_await timer.async_wait(boost::asio::use_awaitable);
        ping_message_.write(boost::beast::websocket::ping_data{});
        begin_write_msg();
      }
    } catch (const boost::system::system_error& e) {
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_http(), "WebSocket 连接发生错误: {}", e.what());
    } catch (const std::exception& e) {
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_http(), "处理 WebSocket 消息发生错误: {}", e.what());
    }
  }

 public:
  explicit data_computers_socket_io_impl(boost::beast::websocket::stream<http::tcp_stream_type> in_stream)
      : web_stream_(std::make_shared<boost::beast::websocket::stream<http::tcp_stream_type>>(std::move(in_stream))),
        strand_(boost::asio::make_strand(g_io_context())) {}

  ~data_computers_socket_io_impl() {}

  boost::beast::websocket::stream<http::tcp_stream_type>& get_web_stream() { return *web_stream_; }

  void set_computer_status(computer_status in_status) { last_status_ = in_status; }
  computer_status get_computer_status() const { return last_status_; }
  uuid get_computer_id() const { return uuid_id_.load(std::memory_order_acquire); }
  // 可在任意执行器上安全调用: 返回允许任务类型的不可变快照(nullptr 表示尚未发布, 等价于不限制)
  std::shared_ptr<const std::set<server_task_info_type>> get_allowed_task_types() const {
    return allowed_task_types_.load(std::memory_order_acquire);
  }

  void run() {
    // 整个连接生命周期(含 init/读写/关闭/ping)都必须跑在本对象的 strand_ 上:
    // web_stream_ 是 boost::beast::websocket::stream, 非线程安全, 且下面的
    // allowed_task_types_ 等状态也依赖单一执行器串行化。
    boost::asio::co_spawn(
        strand_, async_run(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }
  void begin_write_msg() {
    if (writing_ || should_close_) return;
    boost::asio::co_spawn(
        strand_, write_websocket(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }
};

computers_assign_task& computers_assign_task::get_instance() { return *core_set::get_set().computers_assign_task_ptr_; }
boost::asio::awaitable<void> computers_assign_task::register_computer(
    std::shared_ptr<data_computers_socket_io_impl> in_computer
) {
  DOODLE_TO_EXECUTOR(strand_);
  computer_map_[in_computer->get_computer_id()] = in_computer;
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "计算机 {} 注册成功, 当前在线计算机数量 {}", in_computer->get_computer_id(),
      computer_map_.size()
  );
  // 注册后立刻尝试派发: 客户端连接时只发一条注册消息(由 init() 消费), 不会再有第二条消息
  // 触发 run_next_task, 这里不补一次的话刚上线的机器拿不到任务。
  // 若该机注册时上报的是 busy(重连时还有任务在跑), run_next_task 的在线判据会把它跳过。
  co_await run_next_task();
}

void computers_assign_task::clear_offline_computer() {
  for (auto it = computer_map_.begin(); it != computer_map_.end();) {
    if (auto l_ptr = it->second.lock(); l_ptr) {
      ++it;
      continue;
    } else {
      SPDLOG_LOGGER_INFO(
          g_logger_ctrl().get_http(), "清理离线计算机 {}, 当前在线计算机数量 {}", it->first, computer_map_.size()
      );
      it = computer_map_.erase(it);
    }
  }
}

boost::asio::awaitable<void> computers_assign_task::run_next_task_impl(
    std::shared_ptr<data_computers_socket_io_impl> in_computer
) {
  SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "让计算机 {} 执行下一个任务", in_computer->get_computer_id());
  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_jobs = l_sql.get_server_tasks_by_submitted();
  // 过滤：若计算机配置了允许的任务类型，只分配匹配的任务
  // 注意: 这里运行在分配器 strand 上, 而该字段由计算机自己的 strand 更新,
  // 因此只能通过 get_allowed_task_types() 读取原子快照
  if (auto l_allowed = in_computer->get_allowed_task_types(); l_allowed && !l_allowed->empty()) {
    std::erase_if(l_jobs, [&l_allowed](const server_task_info& j) { return !l_allowed->contains(j.type_); });
  }
  if (l_jobs.empty()) {
    in_computer->set_computer_status(computer_status::online);
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(update(l_sql)
                            .from<computer>()
                            .set(c(&computer::status_) = computer_status::online)
                            .where(c(&computer::uuid_id_) == in_computer->get_computer_id()));
    co_await l_sql.run_sql(std::move(l_sqls));
    co_return;
  }
  in_computer->set_computer_status(computer_status::busy);
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<computer>()
                          .set(c(&computer::status_) = computer_status::busy)
                          .where(c(&computer::uuid_id_) == in_computer->get_computer_id()));
  auto l_job               = l_jobs.front();
  l_job.status_            = server_task_info_status::running;
  l_job.run_time_          = {chrono::current_zone(), chrono::system_clock::now()};
  l_job.run_computer_id_   = in_computer->get_computer_id();
  auto& l_ctx              = g_ctx().get<kitsu_ctx_t>();
  auto l_access_token      = jwt::create()
                                 .set_payload_claim("identity_type", jwt::claim{"person"s})
                                 .set_issued_at(chrono::system_clock::now())
                                 .set_id(fmt::to_string(l_job.submitter_))
                                 .set_subject(fmt::to_string(l_job.submitter_))
                                 .set_not_before(chrono::system_clock::now())
                                 .set_expires_at(chrono::system_clock::now() + chrono::days{7})
                                 .sign(jwt::algorithm::hs256{l_ctx.secret_});
  l_job.submitter_cookies_ = l_access_token;
  l_sqls.emplace_back(update(l_sql)
                          .from<server_task_info>()
                          .set(c(&server_task_info::status_) = l_job.status_)
                          .set(c(&server_task_info::run_time_) = l_job.run_time_)
                          .set(c(&server_task_info::run_computer_id_) = l_job.run_computer_id_)
                          .where(c(&server_task_info::uuid_id_) == l_job.uuid_id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  auto l_json = (nlohmann::json{} = l_job);
  in_computer->write_msg(l_json.dump());
  in_computer->begin_write_msg();
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "分发任务 {} 成功，在线计算机 {}", l_job.uuid_id_, in_computer->get_computer_id()
  );
  co_return;
}
boost::asio::awaitable<void> computers_assign_task::run_next_task() {
  DOODLE_TO_EXECUTOR(strand_);
  clear_offline_computer();
  SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "{}", fmt::join(computer_map_ | std::ranges::views::keys, ", "));
  // 判据必须包含服务端自有的事实: 库里是否还有绑在这台机器上的 running 任务。
  // 只信客户端上报的 online 是不够的 —— 客户端在连接/重连时会多发一条 online,
  // 那条过期的 online 会把 run_next_task_impl 刚设的 busy 覆盖掉, 同一台机器就会被派第二个任务。
  auto l_running_computers = get_sqlite_database().get_running_task_computer_ids();
  // 先快照再派发: 循环体里的 co_await 会挂起, 期间同 strand 上的 register_computer /
  // clear_offline_computer 可能插入或删除 computer_map_ 的节点, 直接迭代会拿到失效迭代器
  std::vector<std::shared_ptr<data_computers_socket_io_impl>> l_targets{};
  for (auto& [l_id, l_weak_ptr] : computer_map_) {
    auto l_ptr = l_weak_ptr.lock();
    if (!l_ptr) continue;
    auto l_computer_id = l_ptr->get_computer_id();
    if (l_computer_id.is_nil() || l_ptr->get_computer_status() != computer_status::online) continue;
    if (l_running_computers.contains(l_computer_id)) continue;
    l_targets.emplace_back(std::move(l_ptr));
  }
  for (auto& l_ptr : l_targets) co_await run_next_task_impl(l_ptr);
}
void data_computers::websocket_callback(
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
) {
  person_.check_not_outsourcer();
  auto l_impl = std::make_shared<data_computers_socket_io_impl>(std::move(in_stream));
  l_impl->run();
}
bool data_computers::has_websocket() const { return true; }
}  // namespace doodle::http