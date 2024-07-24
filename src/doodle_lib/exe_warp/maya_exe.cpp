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
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/exe_warp/async_read_pipe.h>
#include <doodle_lib/exe_warp/windows_hide.h>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include <filesystem>
#include <fmt/core.h>
#include <stack>
#include <string>
#include <winnt.h>
#include <winreg/WinReg.hpp>

#include <boost/asio/readable_pipe.hpp>
#include <boost/process.hpp>
#include <boost/process/v2.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace doodle {
namespace {
std::tuple<boost::system::error_code, FSys::path> find_maya_path_impl() {
  try {
    auto l_key_str = fmt::format(LR"(SOFTWARE\Autodesk\Maya\{}\Setup\InstallPath)", core_set::get_set().maya_version);
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
    return {{}, l_maya_path};
  } catch (const winreg::RegException& in_err) {
    return {in_err.code(), {}};
  }
  return {};
}

std::tuple<boost::system::error_code, FSys::path>
install_maya_exe(FSys::path in_maya_path) {
  boost::system::error_code l_ec{};
  FSys::path l_out{};
  try {
    auto l_target_path = FSys::get_cache_path() / "maya" / "exe" / fmt::to_string(core_set::get_set().maya_version) /
                         version::build_info::get().version_str;
    const auto l_run_name = fmt::format("doodle_maya_exe_{}.exe", core_set::get_set().maya_version);
    l_out                 = l_target_path / l_run_name;
    if (!FSys::exists(l_target_path)) FSys::create_directories(l_target_path);

    if (!FSys::exists(l_target_path / "ShadeFragment")) {
      FSys::copy(
        in_maya_path / "bin" / "ShadeFragment", l_target_path / "ShadeFragment",
        FSys::copy_options::recursive | FSys::copy_options::overwrite_existing
      );
    }
    if (!FSys::exists(l_target_path / "ScriptFragment")) {
      FSys::copy(
        in_maya_path / "bin" / "ScriptFragment", l_target_path / "ScriptFragment",
        FSys::copy_options::recursive | FSys::copy_options::overwrite_existing
      );
    }
    if (!FSys::exists(l_out)) {
      FSys::copy(register_file_type::program_location() / l_run_name, l_target_path / l_run_name);
    }
    auto l_program_path = register_file_type::program_location();
    for (auto&& l_it : FSys::directory_iterator(l_program_path)) {
      if (l_it.is_regular_file() && l_it.path().extension() == ".dll") {
        auto l_path_dll = l_target_path / l_it.path().filename();
        if (!FSys::exists(l_path_dll)) FSys::copy(l_it, l_path_dll, FSys::copy_options::overwrite_existing);
      }
    }
  } catch (const FSys::filesystem_error& in_err) {
    l_ec = in_err.code();
  }
  return std::tuple{l_ec, l_out};
}

void add_maya_module() {
  static std::string const k_mod{R"(+ doodle 1.1 .
MYMODULE_LOCATION:= .
PATH+:= plug-ins
PYTHONPATH+:= scripts
)"};
  auto l_maya_plug = register_file_type::program_location().parent_path() / "maya";

  if (!FSys::exists(l_maya_plug / "doodle.mod")) {
    FSys::ofstream k_file{l_maya_plug / "doodle.mod"};
    k_file << k_mod;
  }
}

maya_exe_ns::maya_out_arg get_out_arg(const FSys::path& in_path) {
  if (!FSys::exists(in_path)) return {};

  std::ifstream l_file{in_path};
  auto l_str  = std::string{std::istreambuf_iterator<char>(l_file), std::istreambuf_iterator<char>()};
  auto l_json = nlohmann::json::parse(l_str);
  return l_json.get<maya_exe_ns::maya_out_arg>();
}
}

namespace maya_exe_ns {
FSys::path find_maya_path() {
  auto [l_e, l_maya_path] = find_maya_path_impl();
  if (l_e) throw_error(l_e);
  return l_maya_path;
}
}

boost::asio::awaitable<std::tuple<boost::system::error_code, maya_exe_ns::maya_out_arg>> async_run_maya(
  std::shared_ptr<maya_exe_ns::arg> in_arg,
  logger_ptr in_logger
) {
  auto l_g                 = co_await g_ctx().get<maya_ctx>().queue_->queue(boost::asio::use_awaitable);
  auto [l_e1, l_maya_path] = find_maya_path_impl();
  if (l_e1) {
    in_logger->error("查找Maya路径失败: {}", l_e1.message());
    co_return std::make_tuple(l_e1, maya_exe_ns::maya_out_arg());
  }
  auto [l_e2, l_run_path] = install_maya_exe(l_maya_path);
  if (l_e2) {
    in_logger->error("maya 运行路径转换失败: {}", l_e2.message());
    co_return std::make_tuple(l_e2, maya_exe_ns::maya_out_arg());
  }

  in_arg->maya_path      = l_maya_path;
  in_arg->out_path_file_ = FSys::get_cache_path() / "maya" / "out" / version::build_info::get().version_str /
                           fmt::format("{}.json", core_set::get_set().get_uuid());
  if (!FSys::exists(in_arg->out_path_file_.parent_path()))
    FSys::create_directories(in_arg->out_path_file_.parent_path());
  auto l_arg_path = FSys::write_tmp_file("maya/arg", in_arg->to_json_str(), ".json");

  in_logger->warn("开始写入配置文件 {}", l_arg_path);

  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());

  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    if (l_it.key() != L"PYTHONHOME" && l_it.key() != L"PYTHONPATH")
      l_env.emplace(l_it.key(), l_it.value());
  }

  l_env[L"MAYA_LOCATION"] = l_maya_path.generic_wstring();
  l_env[L"Path"].push_back((l_maya_path / "bin").generic_wstring());
  l_env[L"Path"].push_back(l_run_path.parent_path().generic_wstring());
  l_env[L"MAYA_MODULE_PATH"] = (register_file_type::program_location().parent_path() / "maya").generic_wstring();
  add_maya_module();
  auto l_out_pipe = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  auto l_err_pipe = std::make_shared<boost::asio::readable_pipe>(g_io_context());

  auto l_process_maya = boost::process::v2::process{
      g_io_context(), l_run_path,
      {fmt::format("--{}={}", in_arg->get_arg(), l_arg_path)},
      boost::process::v2::process_stdio{nullptr, *l_out_pipe, *l_err_pipe},
      boost::process::v2::process_environment{l_env},
      boost::process::v2::process_start_dir{l_maya_path / "bin"},
      details::hide_and_not_create_windows
  };
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_out_pipe, in_logger, level::info), boost::asio::detached);
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_err_pipe, in_logger, level::info), boost::asio::detached);

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order,l_ec,l_exit_code, l_ec_t] = co_await boost::asio::experimental::make_parallel_group(
    boost::process::v2::async_execute(
      std::move(l_process_maya), boost::asio::deferred),
    l_timer->async_wait(boost::asio::deferred)
  ).async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) {
        if (!l_ec) l_ec = {l_exit_code, exit_code_category::get()};
        in_logger->error("maya进程返回值错误 {}", l_exit_code);
        co_return std::make_tuple(boost::system::error_code{l_ec}, maya_exe_ns::maya_out_arg{});
      }
      co_return std::tuple{std::move(l_ec), get_out_arg(in_arg->out_path_file_)};
    case 1:
      if (l_ec) {
        in_logger->error("maya 运行超时: {}", l_ec.message());
        co_return std::make_tuple(l_ec, maya_exe_ns::maya_out_arg{});
      }
    default:
      co_return std::make_tuple(boost::system::error_code{boost::asio::error::timed_out},
                                maya_exe_ns::maya_out_arg{});
  }
}
} // namespace doodle