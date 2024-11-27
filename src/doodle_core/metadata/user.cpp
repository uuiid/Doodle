//
// Created by TD on 2021/5/7.
//

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/detail/user_set_data.h>
#include <doodle_core/metadata/user.h>

#include <boost/preprocessor.hpp>

#include "exception/exception.h"
#include "metadata/metadata.h"
#include <entt/entt.hpp>
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
      l_r = entt::handle{*g_reg(), e};
      break;
      DOODLE_LOG_WARN("找到用户名称 {} 的句柄 {}", in_name, l_r);
    }
  }
  return l_r;
}

}  // namespace doodle
