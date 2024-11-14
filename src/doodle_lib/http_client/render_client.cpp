//
// Created by TD on 24-7-26.
//

#include "render_client.h"

#include <doodle_lib/core/http/json_body.h>

#include <boost/asio.hpp>

#include <wil/resource.h>
#include <wil/result.h>
namespace doodle::render_client {

boost::asio::awaitable<void> client::render(
    std::string in_name, FSys::path in_exe_path, std::vector<std::string> in_run_args
) {
  co_return;
}
boost::asio::awaitable<std::vector<computer>> client::get_computers() { co_return std::vector<computer>{}; }
boost::asio::awaitable<std::tuple<std::vector<server_task_info>, std::size_t>> client::get_task(
    std::size_t in_begin, std::size_t in_count
) {
  co_return std::make_tuple(std::vector<server_task_info>(), std::size_t());
}
boost::asio::awaitable<std::string> client::get_logger(boost::uuids::uuid in_uuid, level::level_enum in_level) {
  co_return std::string{};
}

boost::asio::awaitable<void> client::delete_task(boost::uuids::uuid in_uuid) { co_return; }

}  // namespace doodle::render_client