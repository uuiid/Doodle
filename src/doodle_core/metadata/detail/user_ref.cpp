#include "user_ref.h"

#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/user.h"

#include "exception/exception.h"
#include "metadata/metadata.h"

namespace doodle {
void to_json(nlohmann::json& j, const user_ref& p) {
  j["uuid"]       = p.user_ref_attr;
  j["cache_name"] = p.cache_name;
}
void from_json(const nlohmann::json& j, user_ref& p) {
  j.at("uuid").get_to(p.user_ref_attr);
  j.at("cache_name").get_to(p.cache_name);
}
void user_ref::set_uuid(const boost::uuids::uuid& in_data_uuid) {
  user_ref_attr.uuid = in_data_uuid;
  if (auto l_h = user_ref_attr.handle()) {
    handle_cache = l_h;
    cache_name   = l_h.get<user>().get_name();
  }
}
const boost::uuids::uuid& user_ref::get_uuid() const { return user_ref_attr.uuid; }

entt::handle user_ref::user_attr() {
  if (handle_cache && handle_cache.all_of<database, user>() && handle_cache.get<database>() == user_ref_attr &&
      handle_cache.get<user>().get_name() == cache_name) {
    return handle_cache;
  } else {
    auto l_handle = user_ref_attr.handle();
    if (!l_handle) {
      if (cache_name.empty()) {
        cache_name = "null";
      }
      if (auto l_user = user::find_by_user_name(cache_name); l_user) {
        handle_cache = l_user;
        if (handle_cache.any_of<database>()) user_ref_attr = database::ref_data{handle_cache.get<database>()};
        DOODLE_LOG_WARN("按名称寻找到用户 {}", cache_name);
      }

      if (!handle_cache) {
        DOODLE_LOG_WARN("创建临时虚拟用户 {}", cache_name);
        l_handle = make_handle();
        l_handle.emplace<user>(cache_name);
        handle_cache = l_handle;
      }

    } else {
      handle_cache = l_handle;
    }
    return handle_cache;
  }
}
entt::handle user_ref::user_attr() const { return handle_cache; }

void user_ref::user_attr(const entt::handle& in_user) {
  if (!in_user) throw_error(doodle_error{"无效句柄"});
  if (!in_user.all_of<user>()) throw_error(doodle_error{"缺失user组件"});
  handle_cache = in_user;
  cache_name   = in_user.get<user>().get_name();
  if (in_user.all_of<database>()) user_ref_attr = database::ref_data{in_user.get<database>()};
}

}  // namespace doodle