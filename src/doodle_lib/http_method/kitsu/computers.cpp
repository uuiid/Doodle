#include "computers.h"

#include "doodle_core/metadata/computer.h"

#include "doodle_lib/core/app_base.h"
#include "doodle_lib/core/http/http_function.h"
#include "doodle_lib_fwd.h"
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/scope/scope_exit.hpp>

#include <chrono>
#include <fmt/ranges.h>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace doodle::http {
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
  auto l_sql          = get_sqlite_database();
  auto l_computer     = l_sql.get_by_uuid<computer>(computer_id_);
  auto l_json         = in_handle->get_json();
  auto l_computer_ptr = std::make_shared<computer>(l_computer);
  l_json.get_to(*l_computer_ptr);
  l_computer_ptr->hardware_id_         = l_computer.hardware_id_;           // 硬件ID不允许修改
  l_computer_ptr->last_heartbeat_time_ = std::chrono::system_clock::now();  // 更新心跳时间
  co_await l_sql.update(l_computer_ptr);
  socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = computer_id_});
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  co_await l_sql.remove<computer>(computer_id_);
  socket_io::broadcast(socket_io::computer_delete_broadcast_t{.computer_id_ = computer_id_});
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

class data_computers_socket_io_impl : public std::enable_shared_from_this<data_computers_socket_io_impl> {
  std::shared_ptr<boost::beast::websocket::stream<http::tcp_stream_type>> web_stream_;
  std::shared_ptr<computer> computer_;

  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>> message_queue_;
  std::atomic<bool> writing_{false}, should_close_{false};

  // 第一步, 等待计算机发送自身信息, 第二步, 将计算机信息保存到数据库
  boost::asio::awaitable<void> init() {
    boost::beast::flat_buffer l_buffer{};

    co_await web_stream_->async_read(l_buffer);
    auto l_json =
        nlohmann::json::parse(boost::asio::buffers_begin(l_buffer.data()), boost::asio::buffers_end(l_buffer.data()));
    auto l_computer_json            = l_json.get<computer>();
    computer_                       = std::make_shared<computer>(l_computer_json);
    computer_->last_heartbeat_time_ = std::chrono::system_clock::now();
    auto l_sql                      = get_sqlite_database();
    using namespace sqlite_orm;
    if (l_sql.impl_->storage_any_.count<computer>(where(c(&computer::hardware_id_) == computer_->hardware_id_)) != 0) {
      *computer_ =
          l_sql.impl_->storage_any_.get_all<computer>(where(c(&computer::hardware_id_) == computer_->hardware_id_))
              .front();
      computer_->status_ = l_computer_json.status_;
      co_await l_sql.update(computer_);
    } else {
      co_await l_sql.install(computer_);
    }
  }
  void write_msg(const std::string& in_msg) { message_queue_.push(in_msg); }
  friend class computers_assign_task;

  boost::asio::awaitable<void> async_run() {
    co_await init();
    co_await computers_assign_task::get_instance().register_computer(shared_from_this());

    {
      boost::scope::scope_exit l_{[this, sh = shared_from_this()]() { should_close_ = true; }};
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
    }
    co_await web_stream_->async_close(boost::beast::websocket::close_code::normal, boost::asio::use_awaitable);
  }
  boost::asio::awaitable<void> write_websocket() {
    if (writing_ || should_close_) co_return;
    writing_ = true;
    boost::scope::scope_exit l_{[this, sh = shared_from_this()]() { writing_ = false; }};

    while (!message_queue_.empty()) {
      std::string l_msg{};
      message_queue_.pop(l_msg);
      if (!web_stream_) break;
      co_await web_stream_->async_write(boost::asio::buffer(l_msg), boost::asio::use_awaitable);
    }
  }
  boost::asio::awaitable<void> set_computer_status(std::reference_wrapper<computer> in_computer) {
    computer_->status_              = in_computer.get().status_;
    computer_->last_heartbeat_time_ = std::chrono::system_clock::now();
    auto l_sql                      = get_sqlite_database();
    co_await l_sql.update(computer_);
    if (computer_->status_ == computer_status::online) {
      auto l_sql  = get_sqlite_database();
      auto l_jobs = l_sql.get_server_tasks_by_computer_id(computer_->uuid_id_);
      if (l_jobs.empty()) {
        computer_->status_ = computer_status::online;
        co_return;
      }
      auto l_job_ptr       = std::make_shared<server_task_info>(l_jobs.front());
      l_job_ptr->status_   = server_task_info_status::running;
      l_job_ptr->run_time_ = {chrono::current_zone(), chrono::system_clock::now()};
      co_await l_sql.update(l_job_ptr);
      auto l_json = (nlohmann::json{} = *l_job_ptr);
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_http(), "分发任务 {} 成功，在线计算机 {}", l_job_ptr->uuid_id_, computer_->uuid_id_
      );
      write_msg(l_json.dump());
      begin_write_msg();
    }
    socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = computer_->uuid_id_});
  }

 public:
  explicit data_computers_socket_io_impl(boost::beast::websocket::stream<http::tcp_stream_type> in_stream)
      : web_stream_(std::make_shared<boost::beast::websocket::stream<http::tcp_stream_type>>(std::move(in_stream))),
        strand_(boost::asio::make_strand(g_io_context())) {}

  ~data_computers_socket_io_impl() {
    if (!computer_) return;
    if (app_base::Get().is_cancelled()) return;  // 如果是程序退出, 就不更新数据库了,
    // 因为程序退出会导致数据库连接不可用
    auto l_sql                      = get_sqlite_database();
    computer_->status_              = computer_status::offline;
    computer_->last_heartbeat_time_ = std::chrono::system_clock::now();
    l_sql.update_sync(computer_);
    socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = computer_->uuid_id_});
  }

  boost::beast::websocket::stream<http::tcp_stream_type>& get_web_stream() { return *web_stream_; }
  std::shared_ptr<computer> get_computer() const { return computer_; }
  std::shared_ptr<computer> sql_get_computer() {
    if (!computer_)
      return SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_http(), "计算机指针为空, 无法从数据库获取计算机信息"), nullptr;
    auto l_sql = get_sqlite_database();
    *computer_ = l_sql.get_by_uuid<computer>(computer_->uuid_id_);
    return computer_;
  }

  void run() {
    boost::asio::co_spawn(
        g_io_context(), async_run(),
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
  computer_map_.emplace(in_computer->get_computer()->uuid_id_, in_computer);
}
void computers_assign_task::clear_offline_computer() {
  for (auto it = computer_map_.begin(); it != computer_map_.end();) {
    if (auto l_ptr = it->second.lock(); l_ptr) {
      ++it;
      continue;
    } else
      it = computer_map_.erase(it);
  }
}

boost::asio::awaitable<void> computers_assign_task::run_next_task_impl(
    std::shared_ptr<data_computers_socket_io_impl> in_computer
) {
  SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "让计算机 {} 执行下一个任务", in_computer->get_computer()->uuid_id_);
  auto l_sql  = get_sqlite_database();
  auto l_jobs = l_sql.get_server_tasks_by_computer_id(in_computer->get_computer()->uuid_id_);
  if (l_jobs.empty()) {
    in_computer->get_computer()->status_ = computer_status::online;
    co_return;
  }
  auto l_job_ptr       = std::make_shared<server_task_info>(l_jobs.front());
  l_job_ptr->status_   = server_task_info_status::running;
  l_job_ptr->run_time_ = {chrono::current_zone(), chrono::system_clock::now()};
  co_await l_sql.update(l_job_ptr);
  auto l_json = (nlohmann::json{} = *l_job_ptr);
  in_computer->write_msg(l_json.dump());
  in_computer->begin_write_msg();
  SPDLOG_LOGGER_ERROR(
      g_logger_ctrl().get_http(), "分发任务 {} 成功，在线计算机 {}", l_job_ptr->uuid_id_,
      in_computer->get_computer()->uuid_id_
  );
  co_return;
}
boost::asio::awaitable<void> computers_assign_task::run_next_task(uuid in_computer) {
  DOODLE_TO_EXECUTOR(strand_);
  clear_offline_computer();
  SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "{}", fmt::join(computer_map_ | std::ranges::views::keys, ", "));
  if (computer_map_.contains(in_computer)) {
    SPDLOG_LOGGER_INFO(g_logger_ctrl().get_http(), "尝试让计算机 {} 执行下一个任务", in_computer);
    if (auto l_ptr = computer_map_[in_computer].lock(); l_ptr) {
      l_ptr->sql_get_computer();  // 从数据库获取最新的计算机状态
      SPDLOG_LOGGER_INFO(
          g_logger_ctrl().get_http(), "计算机 {} 的状态是 {}, 正在尝试让它执行下一个任务", in_computer,
          l_ptr->get_computer() ? l_ptr->get_computer()->status_ : computer_status::unknown
      );
      if (l_ptr->get_computer() && l_ptr->get_computer()->status_ == computer_status::online) {
        co_await run_next_task_impl(l_ptr);
        co_return;
      }
    }
  }
  SPDLOG_LOGGER_ERROR(
      g_logger_ctrl().get_http(), "让计算机 {} 执行下一个任务失败，未找到在线计算机，计算机将无法继续执行任务",
      in_computer
  );
}

boost::asio::awaitable<void> computers_assign_task::run_next_task() {
  DOODLE_TO_EXECUTOR(strand_);
  clear_offline_computer();
  for (auto& [uuid, weak_ptr] : computer_map_) {
    if (auto l_ptr = weak_ptr.lock(); l_ptr) {
      l_ptr->sql_get_computer();  // 从数据库获取最新的计算机状态
      if (l_ptr->get_computer() && l_ptr->get_computer()->status_ == computer_status::online) {
        co_await run_next_task_impl(l_ptr);
        co_return;
      }
    }
  }
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