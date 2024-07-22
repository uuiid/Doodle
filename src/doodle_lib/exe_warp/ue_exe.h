//
// Created by td_main on 2023/7/26.
//

#pragma once

#include <doodle_core/core/co_queue.h>

#include <doodle_core/core/global_function.h>
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/process.hpp>
#include <boost/process/windows.hpp>
#include <boost/system.hpp>

#include <cstdint>
#include <memory>
#include <stack>
#include <utility>
#include <vector>

namespace doodle {
namespace detail {
struct process_child {
  explicit process_child(::boost::asio::io_context& in_io_context) : out_attr(in_io_context), err_attr(in_io_context) {
  }

  ::boost::process::async_pipe out_attr;
  ::boost::process::async_pipe err_attr;
  ::boost::process::child child_attr{};

  ::boost::asio::streambuf out_str{};
  ::boost::asio::streambuf err_str{};
};
} // namespace detail

class ue_exe {
public:
  struct arg_render_queue;
  struct arg_import_file;
  using any_io_executor = boost::asio::any_io_executor;

private:
  class run_ue_base;
  class run_ue;
  friend class run_ue;

  class run_ue_copy_file;
  friend class run_ue_copy_file;
  FSys::path ue_path_;
  std::stack<std::shared_ptr<run_ue_base>> queue_list_{};
  std::shared_ptr<run_ue_base> run_process_{};
  std::atomic_char16_t run_size_attr{};
  std::weak_ptr<detail::process_child> child_weak_ptr_{};

  void notify_run();

  boost::system::error_code find_ue_exe(const logger_ptr& in_logger);

protected:
  template <typename Handler>
  class wait_handle : public detail::wait_op {
  public:
    explicit wait_handle(Handler&& handler)
      : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(handler)) {
    }

    ~wait_handle() = default;

    static void on_complete(wait_op* op) {
      auto l_self = static_cast<wait_handle*>(op);
      boost::asio::post(boost::asio::prepend(std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_));
    }
  };

  using call_fun_type = std::shared_ptr<detail::wait_op>;
  virtual void queue_up(const entt::handle& in_msg, const std::string& in_command_line, call_fun_type in_call_fun);
  void queue_up(
    const entt::handle& in_msg, const std::vector<std::pair<FSys::path, FSys::path>>& in_command_line,
    call_fun_type in_call_fun
  );

public:
  struct arg_render_queue {
    std::string args_{};
    std::string to_string() const { return args_; };
  };

  struct arg_import_file {
    FSys::path queue_path_{};

    std::vector<FSys::path> import_file_list_{};
    std::string to_string() const;
  };

  ue_exe()          = default;
  virtual ~ue_exe() = default;

  std::string get_file_version(const FSys::path& in_path);

  bool is_run() const { return !child_weak_ptr_.expired(); }

  template <typename CompletionHandler, typename Arg_t>
  std::shared_ptr<detail::process_child> create_child(const Arg_t& in_arg, CompletionHandler&& in_completion) {
    if (ue_path_.empty()) {
      throw_exception(doodle_error{"ue_exe path is empty or not exists"});
    }
    if (!FSys::exists(ue_path_)) {
      throw_exception(doodle_error{"ue_exe path is empty or not exists"});
    }
    auto l_child        = std::make_shared<detail::process_child>(g_io_context());
    l_child->child_attr = ::boost::process::child{
        g_io_context(),
        ::boost::process::cmd     = fmt::format("{} {}", ue_path_, in_arg.to_string()),
        ::boost::process::std_out = l_child->out_attr,
        ::boost::process::std_err = l_child->err_attr,
        ::boost::process::on_exit = in_completion,
        ::boost::process::windows::hide
    };
    child_weak_ptr_ = l_child;
    return l_child;
  };

  template <typename CompletionHandler>
  auto async_run(const entt::handle& in_handle, const std::string& in_arg, CompletionHandler&& in_completion) {
    if (!in_handle.all_of<process_message>()) {
      boost::system::error_code l_ec{error_enum::component_missing_error};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      default_logger_raw()->error("组件缺失 process_message");
      in_completion(l_ec);
      return;
    }
    if (ue_path_.empty()) {
      if (auto l_err = find_ue_exe(in_handle.get<process_message>().logger()); l_err) {
        in_completion(l_err);
        return;
      }
    }

    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
      [this, in_arg, in_handle](auto&& in_completion_handler) {
        auto l_ptr = std::make_shared<wait_handle<std::decay_t<decltype(in_completion_handler)>>>(
          std::forward<decltype(in_completion_handler)>(in_completion_handler)
        );
        this->queue_up(in_handle, in_arg, l_ptr);
      },
      in_completion
    );
  }

  template <typename CompletionHandler>
  auto async_copy_old_project(
    const entt::handle& in_handle, const std::vector<std::pair<FSys::path, FSys::path>>& in_arg,
    CompletionHandler&& in_completion
  ) {
    if (!in_handle.all_of<process_message>()) {
      boost::system::error_code l_ec{error_enum::component_missing_error};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      default_logger_raw()->error("组件缺失 process_message");
      in_completion(l_ec);
      return;
    }

    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
      [this, in_arg, in_handle](auto&& in_completion_handler) {
        auto l_ptr = std::make_shared<wait_handle<std::decay_t<decltype(in_completion_handler)>>>(
          std::forward<decltype(in_completion_handler)>(in_completion_handler)
        );
        this->queue_up(in_handle, in_arg, l_ptr);
      },
      in_completion
    );
  }
};

using ue_exe_ptr = std::shared_ptr<ue_exe>;

class ue_ctx {
public:
  ue_ctx()  = default;
  ~ue_ctx() = default;
  std::shared_ptr<awaitable_queue_limitation> queue_ = std::make_shared<awaitable_queue_limitation>();
};

boost::asio::awaitable<boost::system::error_code> async_run_ue(const std::string& in_arg, logger_ptr in_logger);
} // namespace doodle