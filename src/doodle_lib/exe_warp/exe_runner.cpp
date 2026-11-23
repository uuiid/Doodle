//
// Created by TD on 25-11-23.
//

#include "exe_runner.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exe_warp/async_read_pipe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/platform/win/register_file_type.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/process.hpp>
#include <boost/system.hpp>
#include <winreg/WinReg.hpp>

#include <bit7z/bit7z.hpp>
#include <bit7z/bit7zlibrary.hpp>
#include <bit7z/bitfileextractor.hpp>
#include <bit7z/bitformat.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

namespace doodle {
namespace {
FSys::path find_maya_path_impl() {
  try {
    auto l_key_str = fmt::format(
        LR"(SOFTWARE\\Autodesk\\Maya\\{}\\Setup\\InstallPath)", core_set::get_set().maya_version
    );
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
    return {l_maya_path};
  } catch (const winreg::RegException& in_err) {
    throw_exception(doodle_error{"寻找maya路径失败: {}", in_err.what()});
  }
  return {};
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

boost::asio::awaitable<maya_out_arg> async_run_exe_process_impl(
    const exe_run_process_arg& in_arg, logger_ptr in_logger, std::shared_ptr<server_task_info::run_time_info_t> in_time
) {
  auto l_env = std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value>{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    if (l_it.key() != L"PYTHONHOME" && l_it.key() != L"PYTHONPATH") l_env.emplace(l_it.key(), l_it.value());
  }
  for (auto&& [l_key, l_value] : in_arg.env_) l_env[l_key] = l_value;

  if (in_arg.env_.contains(L"MAYA_MODULE_PATH")) add_maya_module();

  auto l_timer    = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
  auto l_out_pipe = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};
  auto l_err_pipe = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};

  auto l_work_dir = in_arg.work_dir_.empty() ? in_arg.program_.parent_path() : in_arg.work_dir_;

  auto l_process  = boost::process::v2::process{
      g_io_context(),
      in_arg.program_,
      in_arg.args_,
      boost::process::v2::process_stdio{nullptr, l_out_pipe, l_err_pipe},
      boost::process::v2::process_environment{l_env},
      boost::process::v2::process_start_dir{l_work_dir},
      details::hide_and_not_create_windows
  };

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t, l_ec1, l_ec2] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred),
          async_read_pipe(l_out_pipe, in_logger, boost::asio::deferred, level::info, in_arg.out_code_),
          async_read_pipe(l_err_pipe, in_logger, boost::asio::deferred, level::info, in_arg.out_code_)
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

  if (in_time) in_time->end_time_ = std::chrono::system_clock::now();
  switch (l_array_completion_order[0]) {
    case 1:
      throw_exception(doodle_error{"maya 运行超时 {}", l_ec.message()});
    case 0:
    default:
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
          case maya_enum::maya_error_t::namespace_error:
            throw_exception(doodle_error{"maya 中名称空间错误, 引用文件和名称空间不匹配"});
            break;
          case maya_enum::maya_error_t::frame_in_out_error:
            throw_exception(doodle_error{"maya 中帧范围错误, 帧范围超过了动画的实际范围"});
            break;
          default:
            throw_exception(doodle_error{"maya 运行未知错误 {}", l_exit_code});
        }
      }
      break;
  }
  co_return maya_out_arg{};
}


void install_SideFX_Labs(const FSys::path& path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "SideFX_Labs";
  SPDLOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

void install_UnrealEngine5VLC(const FSys::path& path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "UnrealEngine5VLC";
  SPDLOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}
std::string get_ue_plug_version() {
  auto l_doodle_path = core_set::get_set().ue4_path / "Engine" / "Plugins" / "Doodle" / "Doodle.uplugin";
  std::string l_version{};
  if (FSys::exists(l_doodle_path)) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{l_doodle_path});
    if (l_json.contains("VersionName")) l_version = l_json["VersionName"].get<std::string>();
  }
  return l_version;
}
boost::asio::awaitable<bool> installUePath(
    std::shared_ptr<kitsu::kitsu_client> in_kitsu_client, const FSys::path& path, logger_ptr in_logger
) {
  auto l_version_data              = co_await in_kitsu_client->get_ue_plugins_version();
  auto l_ue_version                = core_set::get_set().ue4_version;

  auto [l_version, l_version_list] = l_version_data;
  DOODLE_CHICK(!l_version.empty(), "获取 UE 插件版本失败");
  if (!l_version.starts_with(l_ue_version)) {
    for (auto&& l_v : l_version_list | std::views::reverse) {
      if (l_v.starts_with(l_ue_version)) {
        l_version = l_v;
        break;
      }
    }
  }
  DOODLE_CHICK(
      l_version.starts_with(l_ue_version), "未能找到 UE 插件版本 {} 对应的 UE 版本 {}", l_version, l_ue_version
  );
  SPDLOG_LOGGER_WARN(in_logger, "UE 版本 {} 插件版本 {}", l_ue_version, l_version);
  if (auto l_ver = get_ue_plug_version(); l_version.ends_with(l_ver) && !l_ver.empty()) co_return false;

  auto l_path = co_await in_kitsu_client->get_ue_plugin(l_version);
  DOODLE_CHICK(!l_path.empty(), "获取 UE 插件下载路径失败");
  auto l_out_path = l_path.parent_path() / l_path.stem();
  if (!FSys::exists(l_out_path)) FSys::create_directories(l_out_path);
  bit7z::Bit7zLibrary l_lib{"7zip.dll"};
  bit7z::BitFileExtractor l_extractor{l_lib};
  l_extractor.extract(l_path.generic_string(), l_out_path.generic_string());
  l_out_path /= "Doodle";
  if (!FSys::exists(l_out_path)) throw_exception(doodle_error{"UE 插件解压失败"});

  /// \brief 安装我们自己的插件
  auto targetPath = path / "Engine" / "Plugins" / "Doodle";

  if (FSys::exists(targetPath)) {
    FSys::remove_all(targetPath);
  } else {
    FSys::create_directories(targetPath);
  }

  SPDLOG_INFO(fmt::format("install plug : {} --> {}", l_out_path, targetPath));
  copy(l_out_path, targetPath, FSys::copy_options::recursive | FSys::copy_options::update_existing);
  co_return true;
}

boost::asio::awaitable<void> async_run_ue_impl(
    const ue_run_arg& in_arg, logger_ptr in_logger, std::shared_ptr<kitsu::kitsu_client> in_kitsu_client,
    std::shared_ptr<server_task_info::run_time_info_t> in_time
) {
  in_logger->info("开始检查 UE 版本");
  if (in_time) in_time->start_time_ = std::chrono::system_clock::now();

  in_logger->info("UE 开始检查插件版本");
  if (co_await installUePath(in_kitsu_client, core_set::get_set().ue4_path, in_logger))
    in_logger->info("UE 插件安装完成");
  else
    in_logger->info("UE 插件版本一致, 无需安装");

  auto l_ue_path = core_set::get_set().ue4_path / doodle_config::ue_path_obj;
  if (l_ue_path.empty()) throw_exception(doodle_error{"ue_exe 路径为空, 无法启动UE"});

  in_logger->info("开始运行 ue_exe: {} {}", l_ue_path, in_arg.in_arg_);
  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(co_await boost::asio::this_coro::executor);
  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    l_env.emplace(l_it.key(), l_it.value());
  }
  l_env[L"UE-LocalDataCachePath"]  = std::wstring{L"%GAMEDIR%DerivedDataCache"};
  l_env[L"UE-SharedDataCachePath"] = std::wstring{LR"(\\192.168.10.220\Global Shared DDC Path)"};

  auto l_out_pipe                  = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};
  auto l_err_pipe                  = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};

  auto l_process_ue                = boost::process::v2::process{
      co_await boost::asio::this_coro::executor, l_ue_path, in_arg.in_arg_,
      boost::process::v2::process_stdio{nullptr, l_out_pipe, l_err_pipe}, boost::process::v2::process_environment{l_env}
  };

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t, l_ec1, l_ec2] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process_ue), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred),
          async_read_pipe(l_out_pipe, in_logger, boost::asio::deferred, level::info, "utf-8"),
          async_read_pipe(l_err_pipe, in_logger, boost::asio::deferred, level::info, "utf-8")
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

  if (FSys::path l_prj = ue_exe_ns::find_ue_project_file(in_arg.in_arg_.front()); !l_prj.empty()) {
    auto l_log_path = l_prj.parent_path() / "Saved" / "Logs";
    // 选择最新的日志文件
    FSys::path l_latest_log;
    FSys::file_time_type l_latest_time{};
    if (FSys::exists(l_log_path)) {
      for (auto&& l_file : FSys::directory_iterator(l_log_path)) {
        if (l_file.path().extension() != ".log") continue;
        auto l_time = FSys::last_write_time(l_file.path());
        if (l_time > l_latest_time) {
          l_latest_time = l_time;
          l_latest_log  = l_file.path();
        }
      }
      if (!l_latest_log.empty()) {
        in_logger->info("UE 日志文件路径: {}", l_latest_log.generic_string());
        // 按行读取日志文件内容
        FSys::ifstream l_ifstream{l_latest_log};
        std::string l_line;
        while (std::getline(l_ifstream, l_line)) in_logger->info(l_line);
      }
    }
  }

  if (in_time) in_time->end_time_ = std::chrono::system_clock::now();
  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) throw_exception(doodle_error{"UE进程返回值错误 {}", l_exit_code});
      co_return;
    case 1:
      throw_exception(doodle_error{"UE进程返回值错误 {}", l_exit_code});
    default:
      break;
  }
}
}  // namespace

boost::asio::awaitable<void> exe_runner_prod::run_maya(const maya_run_arg& in_arg, logger_ptr in_logger) {
  auto l_maya_path = find_maya_path_impl() / "bin" / "mayabatch.exe";
  in_logger->warn("开始运行maya {}", l_maya_path);
  auto l_maya_module = (register_file_type::program_location().parent_path() / "maya").generic_wstring();
  exe_run_process_arg l_process_arg{
      .program_  = l_maya_path,
      .args_     = {"-batch", "-hideConsole", "-script", in_arg.mel_file_.generic_string()},
      .work_dir_ = l_maya_path.parent_path(),
      .env_      = {{L"MAYA_MODULE_PATH", l_maya_module}},
      .out_code_ = "GBK",
  };
  co_await run_process(l_process_arg, in_logger);
}

boost::asio::awaitable<maya_out_arg> exe_runner_process::run_process(
    const exe_run_process_arg& in_arg, logger_ptr in_logger
) {
  co_return co_await async_run_exe_process_impl(in_arg, in_logger, {});
}

boost::asio::awaitable<void> exe_runner_process::run_ue(const ue_run_arg& in_arg) {
  co_await async_run_ue_impl(in_arg, in_arg.logger_, in_arg.kitsu_client_, in_arg.time_info_);
}
}  // namespace doodle
