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
  if (!g_reg()->ctx().contains<image_to_move>())
    g_reg()->ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());

  if (!FSys::exists(files_attr)) {
    DOODLE_LOG_INFO("不存在文件 {}", files_attr);
  }
  FSys::ifstream l_file{files_attr};
  entt::handle l_handle{make_handle()};
  auto l_json = nlohmann::json::parse(l_file);
  l_handle.emplace<FSys::path>(l_json["out_path"].get<FSys::path>());
  g_reg()->ctx().at<image_to_move>()->async_create_move(
      l_handle, l_json["image_attr"].get<std::vector<doodle::movie::image_attr>>(),
      [l_w = boost::asio::make_work_guard(g_io_context())]() { app_base::Get().stop_app(); }
  );
}
void create_move_facet::deconstruction() {}
void create_move_facet::add_program_options() {
  opt.add_options()("config_path", boost::program_options::value(&files_attr), "创建视频的序列json选项");
  auto& l_p = program_options::value();
  l_p.add_opt(opt);
}
}  // namespace doodle::facet