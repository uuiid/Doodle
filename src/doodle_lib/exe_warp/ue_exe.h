//
// Created by td_main on 2023/7/26.
//

#pragma once
#include <doodle_core/core/global_function.h>
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
  using any_io_executor = boost::asio::any_io_executor;

 private:
  class run_ue;
  friend class run_ue;
  FSys::path ue_path_;
  std::stack<std::shared_ptr<run_ue>> queue_list_{};
  std::shared_ptr<run_ue> run_process_{};
  std::atomic_char16_t run_size_attr{};
  std::weak_ptr<detail::process_child> child_weak_ptr_{};

  void notify_run();

  void find_ue_exe();

 protected:
  using call_fun_type = boost::asio::any_completion_handler<void(boost::system::error_code)>;
  virtual void queue_up(
      const entt::handle &in_msg, const std::string &in_command_line, call_fun_type in_call_fun,
      const any_io_executor &in_any_io_executor
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
        ::boost::process::on_exit = in_completion,
        ::boost::process::windows::hide
    };
    child_weak_ptr_ = l_child;
    return l_child;
  };

  template <typename CompletionHandler>
  auto async_run(const entt::handle &in_handle, const std::string &in_arg, CompletionHandler &&in_completion) {
    if (!in_handle.all_of<process_message>()) {
      boost::system::error_code l_ec{error_enum::component_missing_error};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      default_logger_raw()->error("组件缺失 process_message");
      in_completion(l_ec);
      return;
    }

    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this, in_arg, in_handle](auto &&in_completion_handler) {
          boost::asio::any_io_executor l_exe = boost::asio::get_associated_executor(in_completion_handler);
          this->queue_up(
              in_handle, in_arg,
              std::move(call_fun_type{std::forward<decltype(in_completion_handler)>(in_completion_handler)}), l_exe
          );
        },
        in_completion
    );
  }
};

using ue_exe_ptr = std::shared_ptr<ue_exe>;

class import_and_render_ue {
 public:
  import_and_render_ue();
  ~import_and_render_ue() = default;

  struct import_files_t {
    std::string type_;
    FSys::path path_;
    friend void to_json(nlohmann::json &j, const import_files_t &p) {
      j["type"] = p.type_;
      j["path"] = p.path_.generic_string();
    }
  };
  struct import_data_t {
    std::string project;
    std::int32_t begin_time;
    std::int32_t end_time;
    std::int32_t episode;
    std::int32_t shot;
    std::string shot_ab;
    FSys::path out_file_dir;
    std::string original_map;
    std::string render_map;
    std::string import_dir;
    std::string level_sequence;
    std::string create_map;
    std::string movie_pipeline_config;

    std::vector<import_files_t> files;
    friend void to_json(nlohmann::json &j, const import_data_t &p) {
      j["project"]               = p.project;
      j["begin_time"]            = p.begin_time;
      j["end_time"]              = p.end_time;
      j["episode"]               = p.episode;
      j["shot"]                  = p.shot;
      j["shot_ab"]               = p.shot_ab;
      j["out_file_dir"]          = p.out_file_dir.generic_string();
      j["original_map"]          = p.original_map;
      j["render_map"]            = p.render_map;
      j["files"]                 = p.files;
      j["import_dir"]            = p.import_dir;
      j["level_sequence"]        = p.level_sequence;
      j["create_map"]            = p.create_map;
      j["movie_pipeline_config"] = p.movie_pipeline_config;
    }
  };

  struct args {
    FSys::path ue_project_path_{};

    import_data_t import_data_{};

    std::string render_map_{};
    std::string render_sequence_{};
    std::string render_config_{};
  };
};

}  // namespace doodle
