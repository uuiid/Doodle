//
// Created by TD on 25-1-23.
//

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/url/url.hpp>

#include <memory>
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
  auto l_woek                               = std::make_shared<http_work>();
  l_set.internal_distributed_render_client_ = l_woek;
  l_woek->run(token_);
  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, delete_) { co_return in_handle->make_msg_204(); }
}  // namespace doodle::http::local