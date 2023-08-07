//
// Created by td_main on 2023/7/26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio/async_result.hpp>
#include <boost/system.hpp>

#include <cstdint>
#include <memory>
#include <stack>
#include <utility>
#include <vector>
namespace doodle {

class ue_exe {
 public:
  struct arg_render_queue;
  struct arg_import_file;

 private:
  class run_ue;
  FSys::path ue_path_;
  std::stack<std::shared_ptr<run_ue>> queue_list_{};
  std::vector<std::shared_ptr<run_ue>> run_process_{};
  std::atomic_char16_t run_size_attr{};

  void notify_run();
  using call_fun_type = std::function<void(boost::system::error_code)>;
  void queue_up(
      const entt::handle &in_msg, const std::string &in_command_line, const std::shared_ptr<call_fun_type> &in_call_fun
  );

  void find_ue_exe();

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

}  // namespace doodle
