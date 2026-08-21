#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/http_method/seedance2/reg.h>

#include "reg.h"
#include <memory>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

/// 画布节点 ---------------------
// /api/canvas/{canvas_id}/node
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node, post) {
  co_return in_handle->make_msg(nlohmann::json{});
}
// /api/canvas/{canvas_id}/node/{node_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, put) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_node_instance, delete_) {
  co_return in_handle->make_msg(nlohmann::json{});
}

/// 画布连线 ---------------------
// /api/canvas/{canvas_id}/connection
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection, post) {
  co_return in_handle->make_msg(nlohmann::json{});
}
// /api/canvas/{canvas_id}/connection/{connection_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_connection_instance, delete_) {
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::seedance2