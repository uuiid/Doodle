#include "doodle_core/exception/exception.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/seed_email.h>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_preview_files_compose_video, post) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "必须上传视频文件");






  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http