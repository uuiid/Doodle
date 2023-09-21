//
// Created by td_main on 2023/7/26.
//

#pragma once
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

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
  explicit process_child(::boost::asio::io_context &in_io_context) : out_attr(in_io_context), err_attr(in_io_context) {}
  ::boost::process::async_pipe out_attr;
  ::boost::process::async_pipe err_attr;
  ::boost::process::child child_attr{};

  ::boost::asio::streambuf out_str{};
  ::boost::asio::streambuf err_str{};
};
}  // namespace detail

class ue_exe {
 public:
  struct arg_render_queue;
  struct arg_import_file;

 private:
  class run_ue;
  FSys::path ue_path_;
  std::stack<std::shared_ptr<run_ue>> queue_list_{};
  std::shared_ptr<run_ue> run_process_{};
  std::atomic_char16_t run_size_attr{};
  std::weak_ptr<detail::process_child> child_weak_ptr_{};

  void notify_run();

  void find_ue_exe();

 protected:
  using call_fun_type = std::function<void(boost::system::error_code)>;
  virtual void queue_up(
      const entt::handle &in_msg, const std::string &in_command_line, const std::shared_ptr<call_fun_type> &in_call_fun
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

  ue_exe() = default;

  std::string get_file_version(const FSys::path &in_path);

  bool is_run() const { return !child_weak_ptr_.expired(); }

  template <typename CompletionHandler, typename Arg_t>
  std::shared_ptr<detail::process_child> create_child(const Arg_t &in_arg, CompletionHandler &&in_completion) {
    find_ue_exe();
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
        ::boost::process::on_exit = [l_child, in_completion = std::forward<decltype(in_completion)>(in_completion)](
                                        int in_exit, const std::error_code &in_error_code
                                    ) { (in_completion)(in_exit, in_error_code); },
        ::boost::process::windows::hide};
    child_weak_ptr_ = l_child;
    return l_child;
  };

  template <typename CompletionHandler, typename Arg_t>
  auto async_run(const entt::handle &in_handle, const Arg_t &in_arg, CompletionHandler &&in_completion) {
    auto l_msg_ref = in_handle.get_or_emplace<process_message>();

    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this, l_msg_ref, l_arg = in_arg.to_string(), in_handle](auto &&in_completion_handler) {
          auto l_fun =
              std::make_shared<call_fun_type>(std::forward<decltype(in_completion_handler)>(in_completion_handler));
          this->queue_up(in_handle, l_arg, l_fun);
        },
        in_completion
    );
  }
};

using ue_exe_ptr = std::shared_ptr<ue_exe>;
}  // namespace doodle
