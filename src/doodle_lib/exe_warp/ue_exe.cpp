//
// Created by td_main on 2023/7/26.
//

#include "ue_exe.h"

#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/logger/logger.h>

#include "boost/asio/read.hpp"
#include "boost/asio/readable_pipe.hpp"
#include "boost/locale/encoding.hpp"
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/windows.hpp>
#include <boost/signals2.hpp>

#include "fmt/core.h"
#include <filesystem>
#include <memory>
#include <string>
namespace doodle {

class ue_exe::run_ue : public std::enable_shared_from_this<ue_exe::run_ue> {
  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};

  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};

 public:
  boost::signals2::scoped_connection cancel_attr{};
  boost::process::child child_attr{};
  ue_exe::call_fun_type call_attr{};
  FSys::path ue_path{};
  std::string arg_attr{};
  logger_ptr logger_attr{};
  ue_exe *self_{};
  void run() {
    if (ue_path.empty() || !FSys::exists(ue_path)) throw_exception(doodle_error{"ue_exe path is empty or not exists"});
    logger_attr->log(log_loc(), level::info, "开始运行 ue_exe: {} {}", ue_path, arg_attr);

    ue_path    = ue_path.lexically_normal();
    child_attr = boost::process::child{
        g_io_context(),
        //        boost::process::exe  = ue_path.generic_string(),
        //        boost::process::args = arg_attr,
        boost::process::cmd = fmt::format("{} {}", ue_path, arg_attr), boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit =
            [this, l_self = shared_from_this()](int in_exit, const std::error_code &in_error_code) {
              logger_attr->log(log_loc(), level::err, "运行结束 ue_exe: {} 退出代码 {}", ue_path, in_exit);
              boost::asio::post(std::bind(std::move(call_attr), in_error_code));
              self_->notify_run();
            },
        boost::process::windows::hide};

    read_out();
    read_err();
  }
  void read_out() {
    boost::asio::async_read_until(
        out_attr, out_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          if (!in_code) {
            /// @brief 此处在主线程调用
            std::string l_ine;
            std::istream l_istream{&out_strbuff_attr};
            std::getline(l_istream, l_ine);
            logger_attr->log(log_loc(), level::info, conv::to_utf<char>(l_ine, "GBK"));
            read_out();
          } else {
            out_attr.close();
            default_logger_raw()->error(conv::to_utf<char>(in_code.what(), "GBK"));
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, err_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          if (!in_code) {
            std::string l_line{};
            std::istream l_istream{&err_strbuff_attr};
            std::getline(l_istream, l_line);
            /// @brief 此处在主线程调用
            logger_attr->log(log_loc(), level::warn, conv::to_utf<char>(l_line, "GBK"));
            read_err();
          } else {
            err_attr.close();
            logger_attr->error(conv::to_utf<char>(in_code.what(), "GBK"));
          }
        }
    );
  }

  void cancel() { child_attr.terminate(); }
};

void ue_exe::notify_run() {
  if (run_process_ && !run_process_->child_attr.running()) run_process_.reset();

  if (!g_ctx().get<program_info>().stop_attr()) {
    if (!queue_list_.empty() && !run_process_) {
      run_process_ = queue_list_.top();
      queue_list_.pop();
      run_process_->run();
    }
  }
}

void ue_exe::queue_up(const entt::handle &in_msg, const std::string &in_command_line, call_fun_type in_call_fun) {
  if (ue_path_.empty()) find_ue_exe();
  auto l_run         = queue_list_.emplace(std::make_shared<run_ue>());
  l_run->ue_path     = ue_path_;
  l_run->arg_attr    = in_command_line;
  l_run->call_attr   = std::move(in_call_fun);
  l_run->self_       = this;
  l_run->logger_attr = in_msg.patch<process_message>().logger();
  l_run->cancel_attr =
      in_msg.patch<process_message>().aborted_sig.connect([l_run_weak_ptr = l_run->weak_from_this()]() {
        if (auto l_ptr = l_run_weak_ptr.lock(); l_ptr) {
          l_ptr->cancel();
        }
      });
  notify_run();
}
void ue_exe::find_ue_exe() {
  auto l_ue_path = core_set::get_set().ue4_path;
  if (l_ue_path.empty()) throw_exception(doodle_error{"ue4 路径未设置"});
  ue_path_ = l_ue_path / doodle_config::ue_path_obj;

  if (!FSys::exists(ue_path_)) {
    throw_exception(doodle_error{"未找到 ue4 程序"});
  }
}
std::string ue_exe::get_file_version(const FSys::path &in_path) {
  auto l_version_path = in_path.parent_path() / "UnrealEditor.version";

  if (!FSys::exists(l_version_path)) {
    throw_exception(doodle_error{"未找到 UnrealEditor.version 文件"});
  }
  FSys::ifstream l_ifstream{l_version_path};
  nlohmann::json const l_json = nlohmann::json::parse(l_ifstream);

  return fmt::format("{}.{}", l_json["MajorVersion"].get<std::int32_t>(), l_json["MinorVersion"].get<std::int32_t>());
}

}  // namespace doodle