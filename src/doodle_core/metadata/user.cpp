//
// Created by TD on 2021/5/7.
//

#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/detail/user_set_data.h>
#include <doodle_core/core/core_set.h>
#include <pin_yin/convert.h>

namespace doodle {

void to_json(nlohmann::json& j, const user& p) {
  j["string_"] = p.p_string_;
}
void from_json(const nlohmann::json& j, user& p) {
  j.at("string_").get_to(p.p_string_);
  p.p_ENUS = convert::Get().toEn(p.p_string_);
}
user::user()
    : p_string_(),
      p_ENUS() {}

user::user(const std::string& in_string)
    : user() {
  set_name(in_string);
}

const std::string& user::get_name() const {
  return p_string_;
}
void user::set_name(const std::string& in_string) {
  p_string_ = in_string;
  p_ENUS    = convert::Get().toEn(p_string_);
}

const std::string& user::get_enus() const {
  return p_ENUS;
}
bool user::operator==(const user& in_rhs) const {
  return p_string_ == in_rhs.p_string_;
}
bool user::operator<(const user& in_rhs) const {
  return p_string_ < in_rhs.p_string_;
}

class user::user_cache {
 public:
  entt::handle user_handle;
  boost::uuids::uuid uuid;

  explicit operator bool() const {
    return user_handle && user_handle.any_of<database>() && user_handle.get<database>() == uuid;
  }
};

entt::handle user::chick_user_reg(entt::registry& in_reg) {
  auto& l_cache = in_reg.ctx().emplace<user::user_cache>();

  if (l_cache.uuid.is_nil()) {
    l_cache.uuid = core_set::get_set().user_id;
  }
  if (!l_cache.user_handle) {
    l_cache.user_handle = database::find_by_uuid(l_cache.uuid);
  }

  if (!l_cache) {
    auto l_create_h = make_handle();
    l_create_h.emplace<user>(in_reg.ctx().at<user>());
    l_create_h.emplace<business::rules>(business::rules::get_default());
    l_cache.uuid        = l_create_h.emplace<database>(l_cache.uuid).uuid();
    l_cache.user_handle = l_create_h;

    database::save(l_create_h);
  }

  DOODLE_CHICK(l_cache, doodle_error{"缺失用户实体{}", l_cache.user_handle});
  return l_cache.user_handle;
}

entt::handle user::get_current_handle() {
  return chick_user_reg(*g_reg());
}

void user::reg_to_ctx(entt::registry& in_reg) {
  auto l_h = chick_user_reg(in_reg);
  /// \brief 在注册表中存在用户
  if (l_h) {
    in_reg.ctx().at<user>() = l_h.get<user>();
  }
}
void user::generate_new_user_id() {
  auto& l_cache   = g_reg()->ctx().emplace<user::user_cache>();
  auto l_create_h = make_handle();
  l_create_h.emplace<user>(g_reg()->ctx().at<user>());
  l_create_h.emplace<business::rules>(business::rules::get_default());
  l_cache.uuid               = l_create_h.emplace<database>().uuid();
  core_set::get_set().user_id = l_cache.uuid;
  l_cache.user_handle        = l_create_h;

  database::save(l_create_h);
}
entt::handle user::find_by_user_name(const std::string& in_name) {
  entt::handle l_r{};
  for (auto&& [e, u] : g_reg()->view<user>().each()) {
    if (u.get_name() == in_name) {
      l_r = make_handle(e);
      break;
      DOODLE_LOG_WARN("找到用户名称 {} 的句柄 {}", in_name, l_r);
    }
  }
  return l_r;
}

}  // namespace doodle
