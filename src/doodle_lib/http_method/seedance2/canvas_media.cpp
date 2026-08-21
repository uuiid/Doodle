#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/http_method/seedance2/reg.h>

#include "reg.h"
#include <memory>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

/// 画布媒体 ---------------------
// /api/canvas/{canvas_id}/node/{node_id}/media
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media, post) {
  co_return in_handle->make_msg(nlohmann::json{});
}
// /api/canvas/{canvas_id}/node/{node_id}/media/{media_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media_instance, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media_instance, put) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media_instance, delete_) {
  co_return in_handle->make_msg(nlohmann::json{});
}
// /api/canvas/{canvas_id}/node/{node_id}/media/{media_id}/file/{role}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media_file, get) {
  co_return in_handle->make_msg(nlohmann::json{});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(canvas_media_file, put) {
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::seedance2