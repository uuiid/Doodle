//
// Created by TD on 2021/12/25.
//

#include "maya_exe.h"

#include "doodle_core/configure/config.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/configure/config.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/filesystem_extend.h>

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

namespace doodle {
namespace {
// 致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
constexpr const auto fatal_error_znch{
    LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab] 中保存)"};

// Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
constexpr const auto fatal_error_en_us{
    LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab])"};

}  // namespace
namespace maya_exe_ns {

FSys::path find_maya_work(const FSys::path &in_file_path) {
  if (FSys::exists(in_file_path.parent_path() / "workspace.mel")) {
    return in_file_path.parent_path();
  }
  if (FSys::exists(in_file_path.parent_path().parent_path() / "workspace.mel")) {
    return in_file_path.parent_path().parent_path();
  }
  return in_file_path.parent_path();
}

class run_maya : public std::enable_shared_from_this<run_maya> {
 public:
  entt::handle mag_attr{};
  FSys::path file_path_attr{};

  nlohmann::json run_script_attr{};
  std::string run_script_attr_key{};
  FSys::path program_path{};

  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};
  boost::process::child child_attr{};

  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};

  std::shared_ptr<std::function<void(boost::system::error_code)>> call_attr{};

  boost::asio::high_resolution_timer timer_attr{g_io_context()};

  run_maya()          = default;
  virtual ~run_maya() = default;

  void run() {
    if (program_path.empty()) throw doodle_error{"没有找到maya路径 (例如 C:/Program Files/Autodesk/Maya2019/bin})"};

    auto l_path = FSys::write_tmp_file(
        "maya", run_script_attr.dump(), ".json", "maya_fun_tool", std::ios::out | std::ios::binary
    );

    auto &&l_msg = mag_attr.get<process_message>();
    l_msg.set_state(l_msg.run);
    l_msg.message(fmt::format("开始写入配置文件 {} \n", l_path), l_msg.warning);
    l_msg.aborted_function = [this]() {
      auto &&l_msg = mag_attr.get<process_message>();
      l_msg.set_state(l_msg.fail);
      l_msg.message("进程被主动结束\n");
      child_attr.terminate();
      cancel();
    };

    timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
    timer_attr.async_wait([this](boost::system::error_code in_code) {
      if (!in_code) {
        auto &&l_msg = mag_attr.get<process_message>();
        l_msg.set_state(l_msg.fail);
        l_msg.message("进程超时，结束任务\n");
        child_attr.terminate();
      } else {
        DOODLE_LOG_ERROR(in_code);
      }
    });

    boost::process::environment l_eve{};
    l_eve["MAYA_LOCATION"] = program_path.parent_path().parent_path().generic_string();
    l_eve["PATH"] += program_path.parent_path().generic_string();

    // boost::process::v2::process_environment l_env{
    //     std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value>{
    //         {"MAYA_LOCATION"s, program_path.parent_path().parent_path().generic_string()},
    //         {"PATH"s, program_path.parent_path().generic_string()}}};

    // boost::asio::readable_pipe l_out_pipe{g_io_context()};
    // boost::asio::readable_pipe l_err_pipe{g_io_context()};
    // boost::process::v2::process_stdio l_io{nullptr, l_out_pipe, l_err_pipe};
    // boost::process::v2::process l_process{
    //     g_io_context(), program_path, {fmt::format("--{}={}", run_script_attr_key, l_path)}, l_env, l_io};
    // boost::asio::cancellation_signal sig{};
    // boost::process::v2::async_execute(
    //     boost::process::v2::process{
    //         g_io_context(), program_path, {fmt::format("--{}={}", run_script_attr_key, l_path)}, l_env, l_io},
    //     boost::asio::bind_cancellation_slot(sig, [](boost::system::error_code ec, int exit_code) {})
    // );
    child_attr = boost::process::child{
        g_io_context(),
        boost::process::exe  = program_path,
        boost::process::args = fmt::format("--{}={}", run_script_attr_key, l_path),
        boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit =
            [this, l_self = shared_from_this()](int in_exit, const std::error_code &in_error_code) {
              timer_attr.cancel();
              auto &&l_msg = mag_attr.get<process_message>();
              l_msg.set_state(in_exit == 0 ? l_msg.success : l_msg.fail);
              l_msg.message(fmt::format("退出代码 {}", in_exit));
              (*call_attr)(in_error_code);
            },
        boost::process::windows::hide,
        boost::process::env = l_eve};

    read_out();
    read_err();
  }

  void read_out() {
    boost::asio::async_read_until(
        out_attr, out_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            /// @brief 此处在主线程调用
            std::string l_ine;
            std::istream l_istream{&out_strbuff_attr};
            std::getline(l_istream, l_ine);
            auto l_str = conv::to_utf<char>(l_ine, "GBK");
            l_msg.progress_step({1, 300});
            l_msg.message(l_str + '\n', l_msg.info);
            read_out();
          } else {
            out_attr.close();
            DOODLE_LOG_ERROR(in_code);
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, err_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            std::string l_line{};
            std::istream l_istream{&err_strbuff_attr};
            std::getline(l_istream, l_line);
            /// @brief 此处在主线程调用
            // 致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
            const static std::wregex l_fatal_error_znch{fatal_error_znch};
            // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
            const static std::wregex l_fatal_error_en_us{fatal_error_en_us};
            auto l_w_str = conv::to_utf<wchar_t>(l_line, "GBK");
            if (std::regex_search(l_w_str, l_fatal_error_znch) || std::regex_search(l_w_str, l_fatal_error_en_us)) {
              DOODLE_LOG_WARN("检测到maya结束崩溃,结束进程: 解算文件是 {}\n", file_path_attr);
              auto l_mstr = fmt::format("检测到maya结束崩溃,结束进程: 解算文件是 {}\n", file_path_attr);
              l_msg.message(l_mstr, l_msg.warning);
              l_msg.set_state(l_msg.fail);
              cancel();
              return;
            } else {
              auto l_str = conv::to_utf<char>(l_line, "GBK");
              l_msg.progress_step({1, 20000});
              l_msg.message(l_str + '\n');
            }

            read_err();
          } else {
            err_attr.close();
            DOODLE_LOG_ERROR(in_code);
          }
        }
    );
  }

  void cancel() {
    timer_attr.cancel();
    child_attr.terminate();
  }
};

}  // namespace maya_exe_ns

class maya_exe::impl {
 public:
  std::stack<std::shared_ptr<maya_exe_ns::run_maya>> run_process_arg_attr;
  std::vector<std::shared_ptr<maya_exe_ns::run_maya>> run_attr{};

  std::atomic_char16_t run_size_attr{};
};

maya_exe::maya_exe() : p_i(std::make_unique<impl>()) {}

FSys::path maya_exe::find_maya_path() const {
  boost::ignore_unused(this);
  auto l_key_str = fmt::format(LR"(SOFTWARE\Autodesk\Maya\{}\Setup\InstallPath)", core_set::get_set().maya_version);
  winreg::RegKey l_key{};
  l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
  auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
  return l_maya_path;
}

void maya_exe::install_maya_exe() {
  auto l_maya_path   = find_maya_path();

  auto l_target_path = FSys::get_cache_path() / fmt::format("maya_{}", core_set::get_set().maya_version) /
                       version::build_info::get().version_str;
  if (!FSys::exists(l_target_path)) FSys::create_directories(l_target_path);

  if (!FSys::exists(l_target_path / "ShadeFragment")) {
    FSys::copy(l_maya_path / "bin" / "ShadeFragment", l_target_path / "ShadeFragment", FSys::copy_options::recursive);
  }
  if (!FSys::exists(l_target_path / "ScriptFragment")) {
    FSys::copy(l_maya_path / "bin" / "ScriptFragment", l_target_path / "ScriptFragment", FSys::copy_options::recursive);
  }
  if (!FSys::exists(l_target_path / fmt::format("doodle_maya_exe_{}.exe", core_set::get_set().maya_version))) {
    FSys::copy(core_set::get_set().program_location(), l_target_path, FSys::copy_options::overwrite_existing);
  }
}

void maya_exe::notify_run() {
  if (!doodle_lib::Get().ctx().get<program_info>().stop_attr())
    while (p_i->run_size_attr < core_set::get_set().p_max_thread && !p_i->run_process_arg_attr.empty()) {
      auto l_run = p_i->run_process_arg_attr.top();
      p_i->run_process_arg_attr.pop();
      ++p_i->run_size_attr;
      l_run->run();
      p_i->run_attr.emplace_back(l_run);
    }

  /// @brief 清除运行完成的程序
  for (auto &&l_i : p_i->run_attr) {
    if (!l_i->child_attr.running()) {
      boost::asio::post(g_io_context(), [l_i, this]() {
        this->p_i->run_attr |= ranges::actions::remove_if([&](auto &&j) -> bool { return l_i == j; });
      });
    }
  }
}
void maya_exe::queue_up(
    const entt::handle &in_msg, const std::string_view &in_key, const nlohmann::json &in_string,
    const std::shared_ptr<call_fun_type> &in_call_fun, const FSys::path &in_run_path
) {
  install_maya_exe();
  DOODLE_CHICK(
      core_set::get_set().has_maya(), doodle_error{"没有找到maya路径 (例如 C:/Program Files/Autodesk/Maya2019/bin})"}
  );
  auto l_run                 = p_i->run_process_arg_attr.emplace(std::make_shared<maya_exe_ns::run_maya>());
  l_run->mag_attr            = in_msg;
  l_run->run_script_attr_key = in_key;
  l_run->run_script_attr     = in_string;
  l_run->file_path_attr      = in_run_path;
  auto &&l_msg               = in_msg.get<process_message>();
  l_msg.set_name(in_run_path.filename().generic_string());
  l_run->call_attr =
      std::make_shared<call_fun_type>([in_call_fun, this, in_msg](const boost::system::error_code &in_code) {
        boost::asio::post(g_io_context(), [=]() {
          --p_i->run_size_attr;
          (*in_call_fun)(in_code);
          this->notify_run();
        });
      });
  notify_run();
}
maya_exe::~maya_exe() {
  for (auto &&l_i : p_i->run_attr) {
    l_i->cancel();
  }
}

}  // namespace doodle
