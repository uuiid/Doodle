//
// Created by TD on 25-1-23.
//

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/url/url.hpp>

#include <memory>

namespace doodle::http::local {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, post) {
  auto l_woek = std::make_shared<http_work>();
  auto& l_set = core_set::get_set();
  l_woek->run(boost::urls::url{l_set.server_ip + "/api/data/computers"});

  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_run, delete_) { co_return in_handle->make_msg_204(); }
}  // namespace doodle::http::local