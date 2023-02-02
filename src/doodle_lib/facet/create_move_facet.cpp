//
// Created by TD on 2023/2/2.
//

#include "create_move_facet.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/logger/logger.h>

#include <doodle_app/app/program_options.h>

#include "doodle_lib/distributed_computing/server.h"
#include "doodle_lib/long_task/image_to_move.h"

#include <boost/program_options.hpp>

// #include <wil/result.h>
namespace doodle::facet {
const std::string& create_move_facet::name() const noexcept { return name_; }
void create_move_facet::operator()() {
  if (!FSys::exists(files_attr)) {
    DOODLE_LOG_INFO("不存在文件 {}", files_attr);
  }
  FSys::ifstream l_file{files_attr};
  entt::handle l_handle{make_handle()};
  auto l_json     = nlohmann::json::parse(l_file);
  auto l_out_path = l_json["out_path"].get<FSys::path>();
  l_handle.emplace<FSys::path>(l_json["out_path"].get<FSys::path>());

  auto l_move = l_json["image_attr"].get<std::vector<doodle::movie::image_attr>>();
  process_message l_msg;
  g_reg()->ctx().at<image_to_move>()->async_create_move(
      l_handle, l_json["image_attr"].get<std::vector<doodle::movie::image_attr>>(),
      [l_w = boost::asio::make_work_guard(g_io_context())](const boost::system::error_code& /*in_error_code*/) {
        app_base::Get().stop_app();
      }
  );
}
void create_move_facet::deconstruction() {}
}  // namespace doodle::facet