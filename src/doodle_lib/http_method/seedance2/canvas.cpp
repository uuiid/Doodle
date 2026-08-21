#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/http_method/seedance2/reg.h>

#include "reg.h"
#include <memory>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

/// 画布 ---------------------
// /api/canvas
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_, post) {
  co_return in_handle->make_msg(nlohmann::json{});
}
// /api/canvas/{canvas_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, put) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_instance, delete_) {
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::seedance2