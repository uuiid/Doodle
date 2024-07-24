//
// Created by td_main on 2023/7/26.
//

#include "ue_exe.h"

#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/logger/logger.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/exe_warp/async_read_pipe.h>

#include <doodle_lib/toolkit/toolkit.h>

#include "boost/asio/readable_pipe.hpp"
#include "boost/locale/encoding.hpp"
#include <boost/asio.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/process/v2.hpp>
#include <boost/system.hpp>

#include "fmt/core.h"
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
      {}, fmt::format("{}.{}", l_json["MajorVersion"].get<std::int32_t>(), l_json["MinorVersion"].get<std::int32_t>())};
}

namespace {
boost::system::error_code chick_ue_plug() {
  auto l_doodle_path = core_set::get_set().ue4_path / "Engine" / "Plugins" / "Doodle" / "Doodle.uplugin";
  std::string l_version{};
  if (FSys::exists(l_doodle_path)) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{l_doodle_path});
    if (l_json.contains("VersionName")) l_version = l_json["VersionName"].get<std::string>();
  }
  if (l_version != version::build_info::get().version_str) {
    try {
      toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
    } catch (const FSys::filesystem_error& error) {
      return error.code();
    }
  }
  return {};
}
}

namespace ue_exe_ns {
std::string get_file_version(const FSys::path& in_path) {
  auto [l_ec, l_str] = get_file_version_impl(in_path);
  if (l_ec) {
    throw_error(l_ec);
  }
  return l_str;
}
}

boost::asio::awaitable<boost::system::error_code> async_run_ue(const std::vector<std::string>& in_arg, logger_ptr in_logger) {
  auto l_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);

  in_logger->info(" 开始检查 UE 版本");
  auto l_e1 = chick_ue_plug();
  if (l_e1) {
    in_logger->error("检查并安装 UE 版本失败: {}", l_e1.message());
    co_return l_e1;
  }
  auto l_ue_path = core_set::get_set().ue4_path / doodle_config::ue_path_obj;
  if (l_ue_path.empty()) {
    in_logger->error("ue_exe 路径为空, 无法启动UE");
    co_return boost::system::error_code{boost::system::errc::no_such_file_or_directory,
                                        boost::system::system_category()};
  }
  in_logger->info("开始运行 ue_exe: {} {}", l_ue_path, in_arg);
  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(co_await boost::asio::this_coro::executor);
  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    l_env.emplace(l_it.key(), l_it.value());
  }
  l_env[L"UE-LocalDataCachePath"]  = std::wstring{L"%GAMEDIR%DerivedDataCache"};
  l_env[L"UE-SharedDataCachePath"] = fmt::format(L"{}\\UE\\DerivedDataCache",
                                                 boost::locale::conv::utf_to_utf<wchar_t>(
                                                   core_set::get_set().depot_ip));

  auto l_out_pipe = std::make_shared<boost::asio::readable_pipe>(co_await boost::asio::this_coro::executor);
  auto l_err_pipe = std::make_shared<boost::asio::readable_pipe>(co_await boost::asio::this_coro::executor);

  auto l_process_ue = boost::process::v2::process{
      co_await boost::asio::this_coro::executor, l_ue_path,
      in_arg,
      boost::process::v2::process_stdio{nullptr, *l_out_pipe, *l_err_pipe},
      boost::process::v2::process_environment{l_env},
      details::hide_and_not_create_windows
  };
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_out_pipe, in_logger, level::info), boost::asio::detached);
  boost::asio::co_spawn(g_io_context(), async_read_pipe(l_err_pipe, in_logger, level::debug), boost::asio::detached);

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout * 5});
  auto [l_array_completion_order,l_ec,l_exit_code, l_ec_t] = co_await boost::asio::experimental::make_parallel_group(
    boost::process::v2::async_execute(
      std::move(l_process_ue), boost::asio::deferred),
    l_timer->async_wait(boost::asio::deferred)
  ).async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));

  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) {
        if (!l_ec) l_ec = {l_exit_code, exit_code_category::get()};
        in_logger->error("UE进程返回值错误 {}", l_exit_code);
        co_return l_ec;
      }
      co_return l_ec;
    case 1:
      if (l_ec) {
        in_logger->error("maya 运行超时: {}", l_ec.message());
        co_return l_ec;
      }
    default:
      co_return boost::system::error_code{boost::asio::error::timed_out};
  }
}
} // namespace doodle