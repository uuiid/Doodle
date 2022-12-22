//
// Created by TD on 2021/5/7.
//

#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/detail/user_set_data.h>
#include <doodle_core/metadata/user.h>

#include <pin_yin/convert.h>

namespace doodle {

void to_json(nlohmann::json& j, const user& p) {
  j["string_"] = p.p_string_;
  j["power"]   = p.power;
}
void from_json(const nlohmann::json& j, user& p) {
  j.at("string_").get_to(p.p_string_);
  p.p_ENUS = convert::Get().toEn(p.p_string_);
  if (j.contains("power")) j.at("power").get_to(p.power);
}

user::user() : p_string_(), p_ENUS() {}

user::user(const std::string& in_string) : user() { set_name(in_string); }

const std::string& user::get_name() const { return p_string_; }
void user::set_name(const std::string& in_string) {
  p_string_ = in_string;
  p_ENUS    = convert::Get().toEn(p_string_);
}

const std::string& user::get_enus() const { return p_ENUS; }
bool user::operator==(const user& in_rhs) const { return p_string_ == in_rhs.p_string_; }
bool user::operator<(const user& in_rhs) const { return p_string_ < in_rhs.p_string_; }

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

user::current_user::operator entt::handle() { return get_handle(); }
entt::handle user::current_user::get_handle() {
  if (!user_handle) {
    user_handle = database::find_by_uuid(uuid);
  }

  if (!*this) {
    auto l_create_h = make_handle();
    l_create_h.emplace<user>(core_set::get_set().user_name);
    l_create_h.emplace<business::rules>(business::rules::get_default());
    uuid        = l_create_h.emplace<database>(uuid).uuid();
    user_handle = l_create_h;

    database::save(l_create_h);
  }

  DOODLE_CHICK(
      user_handle && user_handle.any_of<database>() && user_handle.get<database>() == uuid,
      doodle_error{"缺失用户实体{}", user_handle}
  );
  return user_handle;
}

std::string user::current_user::user_name_attr() {
  if (!*this) get_handle();
  return user_handle.get<user>().get_name();
}
void user::current_user::user_name_attr(const std::string& in_name) {
  if (!*this) get_handle();
  user_handle.get<user>().set_name(in_name);
  core_set::get_set().user_name = in_name;
  database::save(user_handle);
}

user::current_user::operator bool() const {
  return user_handle && user_handle.all_of<database, user>() && user_handle.get<database>() == uuid;
}
user::current_user::current_user() : uuid(core_set::get_set().user_id) {}

user::current_user::~current_user() = default;
void user::current_user::create_user() {
  auto l_create_h = make_handle();
  l_create_h.emplace<user>(core_set::get_set().user_name);
  l_create_h.emplace<business::rules>(business::rules::get_default());
  uuid                        = l_create_h.emplace<database>().uuid();
  core_set::get_set().user_id = uuid;
  user_handle                 = l_create_h;

  database::save(l_create_h);
}

}  // namespace doodle
