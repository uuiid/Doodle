//
// Created by td_main on 2023/7/26.
//

#include "ue_exe.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/exe_warp/async_read_pipe.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include "boost/asio/readable_pipe.hpp"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/process/process.hpp>
#include <boost/scope/scope_exit.hpp>
#include <boost/system.hpp>

#include "fmt/core.h"
#include <bit7z/bit7z.hpp>
#include <bit7z/bit7zlibrary.hpp>
#include <bit7z/bitfileextractor.hpp>
#include <bit7z/bitformat.hpp>
#include <filesystem>
#include <memory>
#include <string>


namespace doodle {

std::tuple<boost::system::error_code, std::string> get_file_version_impl(const FSys::path& in_path) {
  auto l_version_path = in_path.parent_path() / "UnrealEditor.version";

  if (!FSys::exists(l_version_path)) {
    return {{boost::system::errc::no_such_file_or_directory, boost::system::generic_category()}, std::string{}};
  }
  FSys::ifstream l_ifstream{l_version_path};
  nlohmann::json const l_json = nlohmann::json::parse(l_ifstream);

  return {
      {}, fmt::format("{}.{}", l_json["MajorVersion"].get<std::int32_t>(), l_json["MinorVersion"].get<std::int32_t>())
  };
}

FSys::path find_u_pej(const FSys::path& in_path) {
  if (in_path.root_path() == in_path) return {};
  if (!FSys::exists(in_path)) return {};
  if (!FSys::is_directory(in_path)) return find_u_pej(in_path.parent_path());

  for (auto&& l_file : FSys::directory_iterator(in_path)) {
    if (l_file.path().extension() == ".uproject") {
      return l_file.path();
    }
  }
  return find_u_pej(in_path.parent_path());
}

namespace {

void install_SideFX_Labs(const FSys::path& path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "SideFX_Labs";
  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

void install_UnrealEngine5VLC(const FSys::path& path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "UnrealEngine5VLC";
  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

boost::asio::awaitable<void> installUePath(const FSys::path& path) {
  boost::scope::scope_exit l_scope{[]{}};
  auto l_client   = std::make_shared<kitsu::kitsu_client>(core_set::get_set().server_ip);
  auto l_path     = co_await l_client->get_ue_plugin(core_set::get_set().ue4_version);
  auto l_out_path = l_path.parent_path() / l_path.stem();
  if (!FSys::exists(l_out_path)) FSys::create_directories(l_out_path);
  if (l_path.empty()) throw_exception(doodle_error{"获取 UE 插件路径失败"});
  bit7z::Bit7zLibrary l_lib{"7zip.dll"};
  bit7z::BitFileExtractor l_extractor{l_lib};
  l_extractor.extract(l_path.generic_string(), l_out_path.generic_string());
  l_out_path /= "Doodle";
  if (!FSys::exists(l_out_path)) throw_exception(doodle_error{"UE 插件解压失败"});

  /// \brief 安装我们自己的插件
  auto targetPath = path / "Plugins" / "Doodle";

  if (FSys::exists(targetPath)) {
    FSys::remove_all(targetPath);
  } else {
    FSys::create_directories(targetPath);
  }

  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", l_out_path, targetPath));
  copy(l_out_path, targetPath, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

bool chick_ue_plug() {
  auto l_doodle_path = core_set::get_set().ue4_path / "Engine" / "Plugins" / "Doodle" / "Doodle.uplugin";
  std::string l_version{};
  if (FSys::exists(l_doodle_path)) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{l_doodle_path});
    if (l_json.contains("VersionName")) l_version = l_json["VersionName"].get<std::string>();
  }
  return l_version == version::build_info::get().version_str;
}
}  // namespace

namespace ue_exe_ns {
std::string get_file_version(const FSys::path& in_path) {
  auto [l_ec, l_str] = get_file_version_impl(in_path);
  if (l_ec) {
    throw_exception(doodle_error{l_ec.message()});
  }
  return l_str;
}
FSys::path find_ue_project_file(const FSys::path& in_path) {
  if (in_path.empty()) return {};
  return find_u_pej(in_path);
}

boost::asio::awaitable<void> install_doodle_plug(const FSys::path& path) { return installUePath(path); }
}  // namespace ue_exe_ns

boost::asio::awaitable<void> async_run_ue(
    const std::vector<std::string>& in_arg, logger_ptr in_logger, bool create_lock,
    std::shared_ptr<server_task_info::run_time_info_t> in_time
) {
  auto l_g = create_lock ? co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable)
                         : awaitable_queue_limitation::queue_guard_ptr{};

  in_logger->info("开始检查 UE 版本");
  if (in_time) in_time->start_time_ = std::chrono::system_clock::now();
  if (chick_ue_plug()) {
    in_logger->info("UE 插件已经是最新版本, 无需安装");
  } else {
    in_logger->info("UE 插件需要更新, 开始安装");
    co_await ue_exe_ns::install_doodle_plug(core_set::get_set().ue4_path);
    in_logger->info("UE 插件安装完成");
  }

  auto l_ue_path = core_set::get_set().ue4_path / doodle_config::ue_path_obj;
  if (l_ue_path.empty()) throw_exception(doodle_error{"ue_exe 路径为空, 无法启动UE"});

  in_logger->info("开始运行 ue_exe: {} {}", l_ue_path, in_arg);
  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(co_await boost::asio::this_coro::executor);
  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    l_env.emplace(l_it.key(), l_it.value());
  }
  l_env[L"UE-LocalDataCachePath"] = std::wstring{L"%GAMEDIR%DerivedDataCache"};

  auto l_out_pipe                 = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};
  auto l_err_pipe                 = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};

  auto l_process_ue               = boost::process::v2::process{
      co_await boost::asio::this_coro::executor,
      l_ue_path,
      in_arg,
      boost::process::v2::process_stdio{nullptr, l_out_pipe, l_err_pipe},
      boost::process::v2::process_environment{l_env},
      details::hide_and_not_create_windows
  };

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout * 5});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t, l_ec1, l_ec2] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process_ue), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred),
          async_read_pipe(l_out_pipe, in_logger, boost::asio::deferred, level::info, "utf-8"),
          async_read_pipe(l_err_pipe, in_logger, boost::asio::deferred, level::info, "utf-8")
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

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
}  // namespace doodle