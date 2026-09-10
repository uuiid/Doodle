//
// Created by TD on 25-1-23.
//

#include <doodle_core/metadata/server_task_info_type.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/url/url.hpp>

#include <memory>
#include <set>
#include <spdlog/spdlog.h>

namespace doodle::http::local {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, get) {
  auto& l_set           = core_set::get_set();
  bool has_running_task = l_set.internal_distributed_render_client_.lock() != nullptr;
  co_return in_handle->make_msg(nlohmann::json{} = has_running_task);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, post) {
  auto& l_set = core_set::get_set();
  if (auto l_ptr = l_set.internal_distributed_render_client_.lock()) {
    SPDLOG_WARN("已经有一个分布式任务在运行了, 无法同时运行多个分布式任务");
    co_return in_handle->make_msg_204();
  }
  SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "收到运行分布式任务的请求, 将尝试运行分布式任务");

  // 解析允许的任务类型，默认允许 export_fbx 和 auto_light
  std::set<server_task_info_type> l_allowed_task_types{
      server_task_info_type::export_fbx, server_task_info_type::auto_light
  };
  if (auto l_json = in_handle->get_json(); l_json.contains("allowed_task_types")) {
    l_allowed_task_types = l_json.at("allowed_task_types").get<std::set<server_task_info_type>>();
  }

  auto l_woek                               = std::make_shared<http_work>();
  l_set.internal_distributed_render_client_ = l_woek;
  l_woek->run(l_allowed_task_types);
  SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "分布式任务已经开始运行, 允许的任务类型: {}", l_allowed_task_types);
  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, delete_) { co_return in_handle->make_msg_204(); }
}  // namespace doodle::http::local