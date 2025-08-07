//
// Created by TD on 2021/12/25.
//

#include "maya_exe.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/configure/config.h>
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/exe_warp/async_read_pipe.h>
#include <doodle_lib/exe_warp/windows_hide.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/process/process.hpp>

#include <filesystem>
#include <fmt/core.h>
#include <stack>
#include <string>
#include <winnt.h>
#include <winreg/WinReg.hpp>

namespace doodle {
namespace {
FSys::path find_maya_path_impl() {
  try {
    auto l_key_str = fmt::format(LR"(SOFTWARE\Autodesk\Maya\{}\Setup\InstallPath)", core_set::get_set().maya_version);
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
    return {l_maya_path};
  } catch (const winreg::RegException& in_err) {
    throw_exception(doodle_error{"寻找maya路径失败: {}", in_err.what()});
  }
  return {};
}

FSys::path install_maya_exe(FSys::path in_maya_path) {
  static bool is_run{false};
  FSys::path l_out{};
  auto l_target_path = FSys::get_cache_path() / "maya" / "exe" / fmt::to_string(core_set::get_set().maya_version) /
                       version::build_info::get().version_str;
  const auto l_run_name = fmt::format("doodle_maya_exe_{}.exe", core_set::get_set().maya_version);
  l_out                 = l_target_path / l_run_name;
  if (is_run) return l_out;

  if (!FSys::exists(l_target_path)) FSys::create_directories(l_target_path);

  FSys::copy(
      in_maya_path / "bin" / "ShadeFragment", l_target_path / "ShadeFragment",
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );

  FSys::copy(
      in_maya_path / "bin" / "ScriptFragment", l_target_path / "ScriptFragment",
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );

  FSys::copy(
      register_file_type::program_location() / l_run_name, l_target_path / l_run_name,
      FSys::copy_options::update_existing
  );

  auto l_program_path = register_file_type::program_location();
  for (auto&& l_it : FSys::directory_iterator(l_program_path)) {
    if (l_it.is_regular_file() && l_it.path().extension() == ".dll") {
      auto l_path_dll = l_target_path / l_it.path().filename();
      FSys::copy(l_it, l_path_dll, FSys::copy_options::update_existing);
    }
  }
  is_run = true;
  return l_out;
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
}  // namespace

namespace maya_exe_ns {
FSys::path find_maya_path() { return find_maya_path_impl(); }
}  // namespace maya_exe_ns

boost::asio::awaitable<maya_exe_ns::maya_out_arg> async_run_maya(
    std::shared_ptr<maya_exe_ns::arg> in_arg, logger_ptr in_logger,
    std::shared_ptr<server_task_info::run_time_info_t> in_time_info
) {
  auto l_g = co_await g_ctx().get<maya_ctx>().queue_->queue(boost::asio::use_awaitable);
  in_logger->warn("开始运行maya");
  in_time_info->start_time_ = std::chrono::system_clock::now();
  auto l_maya_path          = find_maya_path_impl();

  auto l_this_exe           = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_run_path = install_maya_exe(l_maya_path);
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  auto l_out_path_file_ = FSys::get_cache_path() / "maya" / "out" / version::build_info::get().version_str /
                          fmt::format("{}.json", core_set::get_set().get_uuid());

  in_arg->out_path_file_ = l_out_path_file_;
  auto [l_key, l_args]   = in_arg->get_json_str();
  if (!FSys::exists(l_out_path_file_.parent_path())) FSys::create_directories(l_out_path_file_.parent_path());

  auto l_arg_path = FSys::write_tmp_file("maya/arg", l_args, ".json");

  in_logger->warn("配置命令行 {}", l_arg_path);

  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());

  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    if (l_it.key() != L"PYTHONHOME" && l_it.key() != L"PYTHONPATH") l_env.emplace(l_it.key(), l_it.value());
  }

  l_env[L"MAYA_LOCATION"] = l_maya_path.generic_wstring();
  l_env[L"Path"].push_back((l_maya_path / "bin").generic_wstring());
  l_env[L"Path"].push_back(l_run_path.parent_path().generic_wstring());
  l_env[L"MAYA_MODULE_PATH"] = (register_file_type::program_location().parent_path() / "maya").generic_wstring();
  add_maya_module();
  auto l_out_pipe     = std::make_shared<boost::asio::readable_pipe>(g_io_context());
  auto l_err_pipe     = std::make_shared<boost::asio::readable_pipe>(g_io_context());

  auto l_process_maya = boost::process::v2::process{
      g_io_context(),
      l_run_path,
      {fmt::format("--{}", l_key), fmt::format("--config={}", l_arg_path)},
      boost::process::v2::process_stdio{nullptr, *l_out_pipe, *l_err_pipe},
      boost::process::v2::process_environment{l_env},
      boost::process::v2::process_start_dir{l_maya_path / "bin"},
      details::hide_and_not_create_windows
  };
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_out_pipe, in_logger, level::info), boost::asio::detached);
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_err_pipe, in_logger, level::info), boost::asio::detached);

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process_maya), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred)
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));
  in_time_info->end_time_ = std::chrono::system_clock::now();
  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) {
        switch (maya_enum::maya_error_t{l_exit_code}) {
          case maya_enum::maya_error_t::unknown_error:
            throw_exception(doodle_error{"maya 运行未知错误"});
            break;
          case maya_enum::maya_error_t::camera_name_error:
            throw_exception(doodle_error{"maya 中没有正确的 camera 名字"});
            break;
          case maya_enum::maya_error_t::bone_scale_error:
            throw_exception(doodle_error{"maya 中骨骼有缩放值为 0 的情况"});
            break;
          case maya_enum::maya_error_t::camera_aspect_error:
            throw_exception(doodle_error{"maya 中摄像机的宽高比不正确"});
            break;
          case maya_enum::maya_error_t::cache_path_error:
            throw_exception(doodle_error{"maya 中解算缓存路径不存在"});
            break;
          case maya_enum::maya_error_t::check_error:
            throw_exception(doodle_error{"maya 文件中有错误, 具体请点击查看日志"});
          default:
            throw_exception(doodle_error{"maya 运行未知错误 {}", l_exit_code});
        }
      }
      break;
    case 1:
    default:
      throw_exception(doodle_error{"maya 运行超时 {}", l_ec.message()});
      break;
  }
  co_return get_out_arg(l_out_path_file_);
}
}  // namespace doodle