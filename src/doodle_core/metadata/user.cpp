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
void user::reg_to_ctx(entt::registry& in_reg) {
  /// \brief 开始兼容旧版本的设置
  database::ref_data l_ref{};
  /// \brief 有保存的用户数据
  if (core_set::getSet().json_data->contains("user_data")) {
    l_ref = core_set::getSet().json_data->at("user_data").get<database::ref_data>();
  }

  /// \brief 在注册表中存在用户
  if (auto l_h = l_ref.handle();
      l_h) {
    in_reg.ctx().at<user>()            = l_h.get<user>();
    in_reg.ctx().at<business::rules>() = l_h.get<business::rules>();
  } else {  /// \brief 在注册表中不存在用户
    detail::user_set_data l_user_data{};
    l_user_data.user_data              = in_reg.ctx().at<user>();

    in_reg.ctx().at<user>()            = l_user_data.user_data;
    in_reg.ctx().at<business::rules>() = l_user_data.rules_attr;

    auto l_create_h                    = make_handle();
    l_create_h.emplace<user>(l_user_data.user_data);
    l_create_h.emplace<database>(std::move(l_user_data.data_ref));
    l_create_h.emplace<business::rules>(l_user_data.rules_attr);
    database::save(l_create_h);
  }
}

}  // namespace doodle
