//
// Created by TD on 25-5-8.
//
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/label.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "model_library.h"
namespace doodle::http::model_library {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(context, get) {
  person_.check_user();
  nlohmann::json l_json{};
  l_json["tree_nodes"] = get_sqlite_database().get_all<assets_helper::database_t>();
  co_return in_handle->make_msg(l_json);
}

}  // namespace doodle::http::model_library