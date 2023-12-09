//
// Created by TD on 2021/12/25.
//

#include "maya_exe.h"

#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/configure/config.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/filesystem_extend.h>

#include "boost/process/start_dir.hpp"
#include "boost/signals2/connection.hpp"
#include <boost/asio.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include "exe_warp/maya_exe.h"
#include <filesystem>
#include <fmt/core.h>
#include <stack>
#include <string>
#include <winnt.h>
#include <winreg/WinReg.hpp>
// #include <type_traits>

#include <boost/asio/readable_pipe.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/process.hpp>
#include <boost/process/environment.hpp>
#include <boost/process/extend.hpp>
// #include <boost/process/v2/environment.hpp>
// #include <boost/process/v2/execute.hpp>
// #include <boost/process/v2/process.hpp>
// #include <boost/process/v2/stdio.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>
#elif defined __linux__
#include <boost/process/posix.hpp>
#endif
// #define BOOST_PROCESS_V2_SEPARATE_COMPILATION

// #include <boost/process/v2.hpp>
// #include <boost/process/v2/src.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
namespace doodle {

namespace maya_exe_ns {

class run_maya : public std::enable_shared_from_this<run_maya>, public maya_exe_ns::maya_process_base {
 public:
  FSys::path file_path_attr{};

  nlohmann::json run_script_attr{};
  std::string run_script_attr_key{};
  FSys::path program_path{};
  FSys::path maya_program_path{};

  boost::process::child child_attr{};

  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};
  logger_ptr log_attr{};

  maya_exe::call_fun_type call_attr{};

  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};
  boost::asio::high_resolution_timer timer_attr{g_io_context()};
  boost::signals2::scoped_connection cancel_attr{};
  boost::asio::any_io_executor any_io_executor_;
  bool is_cancel{};

  explicit run_maya(maya_exe *in_maya_exe) : maya_exe_ns::maya_process_base(in_maya_exe) {}
  ~run_maya() override = default;

  bool running() override { return child_attr.running(); }

  void run() override {
    if (is_cancel) {
      log_attr->log(log_loc(), level::err, "用户结束: {}", run_script_attr.dump());
      boost::system::error_code l_ec{boost::asio::error::operation_aborted};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      boost::asio::post(any_io_executor_, std::bind(std::move(call_attr), l_ec));
      next_run();
      return;
    }

    if (program_path.empty()) {
      boost::system::error_code l_ec{error_enum::file_not_exists};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      log_attr->log(log_loc(), level::warn, "没有找到maya文件");
      boost::asio::post(any_io_executor_, std::bind(std::move(call_attr), l_ec));
      return;
    }

    auto l_path = FSys::write_tmp_file("maya", run_script_attr.dump(), ".json");

    log_attr->log(log_loc(), level::warn, "开始写入配置文件 {}", l_path);

    timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
    timer_attr.async_wait([this](boost::system::error_code in_code) {
      if (!in_code) {
        log_attr->log(log_loc(), level::warn, "进程超时，结束任务");
        child_attr.terminate();
      } else {
        log_attr->log(log_loc(), level::warn, in_code);
      }
    });

    boost::process::environment l_eve = boost::this_process::environment();
    l_eve["MAYA_LOCATION"]            = maya_program_path.generic_string();
    l_eve["Path"] += (maya_program_path / "bin").generic_string();
    l_eve["Path"] += program_path.parent_path().generic_string();
    l_eve["Path"] += core_set::get_set().program_location().generic_string();
    child_attr = boost::process::child{
        g_io_context(),
        boost::process::exe       = program_path.generic_string(),
        boost::process::start_dir = (maya_program_path / "bin").generic_string(),
        boost::process::args      = fmt::format("--{}={}", run_script_attr_key, l_path),
        boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit =
            [this, l_self = shared_from_this()](int in_exit, const std::error_code &in_error_code) {
              timer_attr.cancel();
              log_attr->log(log_loc(), level::warn, "进程结束 {}", in_exit);
              boost::asio::post(any_io_executor_, std::bind(std::move(call_attr), in_error_code));
              next_run();
            },
        boost::process::windows::hide,
        l_eve
    };

    read_out();
    read_err();
  }

  void read_out() {
    boost::asio::async_read_until(
        out_attr, out_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            /// @brief 此处在主线程调用
            std::string l_ine;
            std::istream l_istream{&out_strbuff_attr};
            std::getline(l_istream, l_ine);
            auto l_str = conv::to_utf<char>(l_ine, "GBK");
            log_attr->log(log_loc(), level::info, l_ine);
            read_out();
          } else {
            log_attr->log(log_loc(), level::warn, in_code);
            out_attr.close();
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, err_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            std::string l_line{};
            std::istream l_istream{&err_strbuff_attr};
            std::getline(l_istream, l_line);
            auto l_str = conv::to_utf<char>(l_line, "GBK");
            log_attr->log(log_loc(), level::info, l_line);
            read_err();
          } else {
            err_attr.close();
            log_attr->log(log_loc(), level::warn, in_code);
          }
        }
    );
  }

  void cancel() {
    log_attr->log(log_loc(), level::warn, "进程被主动结束");
    timer_attr.cancel();
    child_attr.terminate();
    is_cancel = true;
  }
};

}  // namespace maya_exe_ns

class maya_exe::impl {
 public:
  std::stack<std::shared_ptr<maya_exe_ns::maya_process_base>> run_process_arg_attr;
  std::vector<std::shared_ptr<maya_exe_ns::maya_process_base>> run_attr{};

  std::atomic_char16_t run_size_attr{};
  FSys::path run_path{};
};

void maya_exe_ns::maya_process_base::next_run() {
  --maya_exe_->p_i->run_size_attr;
  maya_exe_->notify_run();
}

maya_exe::maya_exe() : p_i(std::make_unique<impl>()) {}

FSys::path maya_exe::find_maya_path() const {
  boost::ignore_unused(this);
  auto l_key_str = fmt::format(LR"(SOFTWARE\Autodesk\Maya\{}\Setup\InstallPath)", core_set::get_set().maya_version);
  DOODLE_LOG_INFO("开始寻找在注册表中寻找maya");
  winreg::RegKey l_key{};
  l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
  auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
  return l_maya_path;
}

void maya_exe::install_maya_exe() {
  try {
    auto l_maya_path   = find_maya_path();

    auto l_target_path = FSys::get_cache_path() / fmt::format("maya_{}", core_set::get_set().maya_version) /
                         version::build_info::get().version_str;
    const auto l_run_name = fmt::format("doodle_maya_exe_{}.exe", core_set::get_set().maya_version);
    p_i->run_path         = l_target_path / l_run_name;
    if (!FSys::exists(l_target_path)) FSys::create_directories(l_target_path);

    if (!FSys::exists(l_target_path / "ShadeFragment")) {
      FSys::copy(
          l_maya_path / "bin" / "ShadeFragment", l_target_path / "ShadeFragment",
          FSys::copy_options::recursive | FSys::copy_options::overwrite_existing
      );
    }
    if (!FSys::exists(l_target_path / "ScriptFragment")) {
      FSys::copy(
          l_maya_path / "bin" / "ScriptFragment", l_target_path / "ScriptFragment",
          FSys::copy_options::recursive | FSys::copy_options::overwrite_existing
      );
    }
    if (!FSys::exists(p_i->run_path)) {
      FSys::copy(core_set::get_set().program_location() / l_run_name, l_target_path / l_run_name);
    }
  } catch (const winreg::RegException &in_err) {
    DOODLE_LOG_WARN("在注册表中寻找maya失败,错误信息: {}", boost::diagnostic_information(in_err));
  }
}

void maya_exe::notify_run() {
  if (!g_ctx().get<program_info>().stop_attr())
    while (p_i->run_size_attr < core_set::get_set().p_max_thread && !p_i->run_process_arg_attr.empty()) {
      auto l_run = p_i->run_process_arg_attr.top();
      p_i->run_process_arg_attr.pop();
      ++p_i->run_size_attr;
      l_run->run();
      p_i->run_attr.emplace_back(l_run);
    }

  /// @brief 清除运行完成的程序
  for (auto &&l_i : p_i->run_attr) {
    if (!l_i->running()) {
      boost::asio::post(g_io_context(), [l_i, this]() {
        this->p_i->run_attr |= ranges::actions::remove_if([&](auto &&j) -> bool { return l_i == j; });
      });
    }
  }
}
void maya_exe::queue_up(
    const entt::handle &in_msg, const std::string_view &in_key, const nlohmann::json &in_string,
    call_fun_type in_call_fun, const any_io_executor &in_any_io_executor, const FSys::path &in_run_path
) {
  install_maya_exe();

  auto l_run = std::dynamic_pointer_cast<maya_exe_ns::run_maya>(
      p_i->run_process_arg_attr.emplace(std::make_shared<maya_exe_ns::run_maya>(this))
  );
  l_run->run_script_attr_key = in_key;
  l_run->run_script_attr     = in_string;
  l_run->file_path_attr      = in_run_path;
  l_run->program_path        = p_i->run_path;
  l_run->maya_program_path   = find_maya_path();
  l_run->call_attr           = std::move(in_call_fun);
  l_run->log_attr            = in_msg.get<process_message>().logger();
  l_run->any_io_executor_    = in_any_io_executor;
  l_run->cancel_attr = in_msg.get<process_message>().aborted_sig.connect([l_run_weak_ptr = l_run->weak_from_this()]() {
    if (auto l_ptr = l_run_weak_ptr.lock(); l_ptr) l_ptr->cancel();
  });
  notify_run();
}
maya_exe::~maya_exe() = default;

}  // namespace doodle
