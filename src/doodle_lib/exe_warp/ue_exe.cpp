//
// Created by td_main on 2023/7/26.
//

#include "ue_exe.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/system.hpp>

#include "core/file_sys.h"
#include <filesystem>
#include <fmt/core.h>
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

namespace ue_exe_ns {
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

}  // namespace ue_exe_ns

boost::asio::awaitable<void> async_run_ue(
    const std::vector<std::string>& in_arg, logger_ptr in_logger, std::shared_ptr<kitsu::kitsu_client> in_kitsu_client,
    bool create_lock, std::shared_ptr<server_task_info::run_time_info_t> in_time
) {
  auto l_g      = create_lock ? co_await core_set::get_set().ue_ctx_->queue_->queue(boost::asio::use_awaitable)
                              : awaitable_queue_limitation::queue_guard_ptr{};
  auto l_runner = core_set::get_set().exe_runner_;
  if (!l_runner) throw_exception(doodle_error{"exe_runner_ 未初始化"});
  co_await l_runner->run_ue(ue_run_arg{in_arg, in_logger, in_kitsu_client, in_time});
}
}  // namespace doodle
