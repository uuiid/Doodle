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
  boost::signals2::scoped_connection cancel_attr{};

 public:
  boost::process::child child_attr{};
  std::shared_ptr<std::function<void(boost::system::error_code)>> call_attr{};
  FSys::path ue_path{};
  entt::handle mag_attr{};
  std::string arg_attr{};
  void run() {
    if (ue_path.empty() || !FSys::exists(ue_path)) throw_exception(doodle_error{"ue_exe path is empty or not exists"});

    auto &l_mag = mag_attr.patch<process_message>();

    l_mag.set_state(l_mag.run);
    l_mag.message(fmt::format("开始运行 ue_exe: {}", ue_path.string()));

    cancel_attr = l_mag.aborted_sig.connect([this]() {
      auto &&l_msg = mag_attr.get<process_message>();
      l_msg.set_state(l_msg.fail);
      l_msg.message("进程被主动结束\n");
      cancel();
    });

    child_attr  = boost::process::child{
        g_io_context(),
        boost::process::exe  = ue_path.generic_string(),
        boost::process::args = arg_attr,
        boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit =
            [this, l_self = shared_from_this()](int in_exit, const std::error_code &in_error_code) {
              auto &&l_msg = mag_attr.get<process_message>();
              l_msg.set_state(in_exit == 0 ? l_msg.success : l_msg.fail);
              l_msg.message(fmt::format("退出代码 {}", in_exit));
              (*call_attr)(in_error_code);
            },
        boost::process::windows::hide};

    read_out();
    read_err();
  }
  void read_out() {
    boost::asio::async_read_until(
        out_attr, out_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          if (!in_code) {
            /// @brief 此处在主线程调用
            std::string l_ine;
            std::istream l_istream{&out_strbuff_attr};
            std::getline(l_istream, l_ine);
            l_msg.progress_step({1, 300});
            l_msg.message(l_ine + '\n', l_msg.info);
            read_out();
          } else {
            out_attr.close();
            DOODLE_LOG_ERROR(in_code.what());
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, err_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          if (!in_code) {
            std::string l_line{};
            std::istream l_istream{&err_strbuff_attr};
            std::getline(l_istream, l_line);
            /// @brief 此处在主线程调用
            l_msg.progress_step({1, 20000});
            l_msg.message(l_line + '\n');
            read_err();
          } else {
            err_attr.close();
            DOODLE_LOG_ERROR(in_code.what());
          }
        }
    );
  }

  void cancel() { child_attr.terminate(); }
};

void ue_exe::notify_run() {
  if (!doodle_lib::Get().ctx().get<program_info>().stop_attr())
    while (run_size_attr < core_set::get_set().p_max_thread && !queue_list_.empty()) {
      auto l_run = queue_list_.top();
      queue_list_.pop();
      ++run_size_attr;
      l_run->run();
      run_process_.emplace_back(l_run);
    }

  /// @brief 清除运行完成的程序
  for (auto &&l_i : run_process_) {
    if (!l_i->child_attr.running()) {
      boost::asio::post(g_io_context(), [l_i, this]() {
        this->run_process_ |= ranges::actions::remove_if([&](auto &&j) -> bool { return l_i == j; });
      });
    }
  }
}

void ue_exe::queue_up(
    const entt::handle &in_msg, const std::string &in_command_line, const std::shared_ptr<call_fun_type> &in_call_fun
) {
  auto l_run       = queue_list_.emplace(std::make_shared<run_ue>());
  l_run->ue_path   = ue_path_;
  l_run->mag_attr  = in_msg;
  l_run->arg_attr  = in_command_line;
  l_run->call_attr = in_call_fun;
  l_run->run();

  l_run->call_attr =
      std::make_shared<call_fun_type>([in_call_fun, this, in_msg](const boost::system::error_code &in_code) {
        boost::asio::post(g_io_context(), [=]() {
          --run_size_attr;
          (*in_call_fun)(in_code);
          this->notify_run();
        });
      });
  notify_run();
}

std::string ue_exe::arg_render_queue::to_string() const { return std::string(); }
std::string ue_exe::arg_import_file::to_string() const { return std::string(); }
}  // namespace doodle