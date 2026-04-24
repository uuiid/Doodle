#include "doodle_core/metadata/task.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "http_method/kitsu.h"
#include "reg.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <regex>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

namespace {}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, post) {
  auto l_json = in_handle->get_json();

  auto l_task = std::make_shared<sd2::task>();
  l_json.get_to(*l_task);
  l_task->user_id_      = person_.person_.uuid_id_;
  l_task->ai_studio_id_ = person_.get_ai_studio_id();
  auto l_seed_json      = l_task->data_request_;
  // 查找 以https://或者http://开头的url，并替换host部分为空
  static std::regex l_url_regex(R"(https?:\/\/[^\/\s]+)");
  for (auto&& l_value : l_task->data_request_.at("content")) {
    nlohmann::json* l_url{};
    if (l_value.contains("image_url"))
      l_url = &l_value.at("image_url").at("url");
    else if (l_value.contains("video_url"))
      l_url = &l_value.at("video_url").at("url");
    else
      continue;
    *l_url         = std::regex_replace(l_url->get<std::string>(), l_url_regex, "");
  }
  auto l_sql = get_sqlite_database();
  co_await l_sql.install(l_task);

  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::user_id_) == person_.person_.uuid_id_));
  co_return in_handle->make_msg(l_vec);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec =
      l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::ai_studio_id_) == person_.get_ai_studio_id()));
  co_return in_handle->make_msg(l_vec);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  co_return in_handle->make_msg(l_task);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task_instance, delete_) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  co_await l_sql.remove<sd2::task>(id_);
  co_return in_handle->make_msg(nlohmann::json{{"id", id_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_thumbnail_task, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_thumbnail_task_file(id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "缩略图不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_pictures_task, get) {
  auto l_sql  = get_sqlite_database();
  auto l_task = l_sql.get_by_uuid<sd2::task>(id_);
  DOODLE_CHICK_HTTP(l_task.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足")

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_file = l_ctx.get_sd2_pictures_task_file(id_, l_task.file_extension_);
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "图片或者视频不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
}  // namespace doodle::http::seedance2