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
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, post) {
  static std::weak_ptr<http_work> work_ptr_{};
  if (auto l_ptr = work_ptr_.lock()) {
    SPDLOG_WARN("已经有一个分布式任务在运行了, 无法同时运行多个分布式任务");
    co_return in_handle->make_msg_204();
  }
  auto l_woek = std::make_shared<http_work>();
  work_ptr_ = l_woek;
  auto& l_set = core_set::get_set();
  l_woek->run(token_);

  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, delete_) { co_return in_handle->make_msg_204(); }
}  // namespace doodle::http::local