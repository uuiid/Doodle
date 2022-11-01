//
// Created by TD on 2021/5/7.
//

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/pin_yin/convert.h>

#include <core/core_set.h>

namespace doodle {

class assets_file::impl {
 public:
  database::ref_data ref_user{};

  entt::handle handle_cache{};

  FSys::path path;
  std::string p_name;
  std::uint64_t p_version{};

  std::string organization_p;

  /// \brief 不要使用, 已经是过期段属性 保留只是兼容性更改
  std::string p_user;
};

void to_json(nlohmann::json& j, const assets_file& p) {
  j["name"]           = p.p_i->p_name;
  j["organization_p"] = p.p_i->organization_p;
  j["version"]        = p.p_i->p_version;
  j["path"]           = p.p_i->path;

  j["user_ref"]       = p.p_i->ref_user;
  j["user"]           = p.p_i->p_user;
}
void from_json(const nlohmann::json& j, assets_file& p) {
  if (j.contains("user"))
    j.at("user").get_to(p.p_i->p_user);

  j.at("name").get_to(p.p_i->p_name);
  j.at("version").get_to(p.p_i->p_version);
  if (j.contains("organization_p"))
    j.at("organization_p").get_to(p.p_i->organization_p);
  if (j.contains("path"))
    j.at("path").get_to(p.p_i->path);

  if (j.contains("user_ref"))
    j.at("user_ref").get_to(p.p_i->ref_user);
}

assets_file::assets_file()
    : p_i(std::make_unique<impl>()){};

assets_file::assets_file(const FSys::path& in_path)
    : assets_file() {
  p_i->path   = in_path;
  p_i->p_name = in_path.stem().generic_string();
  user_attr(g_reg()->ctx().at<user::current_user>().get_handle());
  p_i->organization_p = core_set::get_set().organization_name;
}

assets_file::assets_file(const FSys::path& in_path, std::string in_name, std::uint64_t in_version)
    : assets_file() {
  p_i->path           = in_path;
  p_i->p_name         = std::move(in_name);
  p_i->p_version      = in_version;
  p_i->organization_p = core_set::get_set().organization_name;
  user_attr(g_reg()->ctx().at<user::current_user>().get_handle());
}

std::string assets_file::str() const {
  return p_i->p_name;
}
const std::string& assets_file::name_attr() const {
  return p_i->p_name;
}
bool assets_file::operator<(const assets_file& in_rhs) const {
  return std::tie(p_i->p_name, p_i->p_version) < std::tie(in_rhs.p_i->p_name, in_rhs.p_i->p_version);
}
bool assets_file::operator==(const assets_file& in_rhs) const {
  return std::tie(p_i->p_name, p_i->p_version) == std::tie(in_rhs.p_i->p_name, in_rhs.p_i->p_version);
}

entt::handle assets_file::user_attr() const {
  if (p_i->handle_cache &&
      p_i->handle_cache.all_of<database, user>() &&
      p_i->handle_cache.get<database>() == p_i->ref_user &&
      p_i->handle_cache.get<user>().get_name() == p_i->p_user) {
    return p_i->handle_cache;
  } else {
    auto l_handle = p_i->ref_user.handle();
    if (!l_handle) {
      if (p_i->p_user.empty()) {
        p_i->p_user = "null";
      }
      if (auto l_user = user::find_by_user_name(p_i->p_user);
          l_user) {
        p_i->handle_cache = l_user;
        if (p_i->handle_cache.any_of<database>()) p_i->ref_user = database::ref_data{p_i->handle_cache.get<database>()};
        DOODLE_LOG_WARN("按名称寻找到用户 {}", p_i->p_user);
      }

      if (!p_i->handle_cache) {
        DOODLE_LOG_WARN("创建临时虚拟用户 {}", p_i->p_user);
        l_handle = make_handle();
        l_handle.emplace<user>(p_i->p_user);
        p_i->handle_cache = l_handle;
      }

    } else {
      p_i->handle_cache = l_handle;
    }
    return p_i->handle_cache;
  }
}
void assets_file::user_attr(const entt::handle& in_user) {
  DOODLE_CHICK(in_user.any_of<user>(), doodle_error{"句柄 {} 缺失必要组件 user", in_user});
  if (in_user.any_of<database>())
    p_i->ref_user = database::ref_data{in_user.get<database>()};
  else
    p_i->ref_user = {};
  p_i->handle_cache = in_user;
  p_i->p_user       = in_user.get<user>().get_name();
}

const std::uint64_t& assets_file::version_attr() const noexcept {
  return p_i->p_version;
}

void assets_file::version_attr(const std::uint64_t& in_Version) noexcept {
  p_i->p_version = in_Version;
}
FSys::path assets_file::get_path_normal() const {
  DOODLE_CHICK(g_reg()->ctx().contains<project>(), doodle_error{"缺失项目上下文"});
  if (p_i->path.has_root_path())
    return p_i->path;
  else {
    auto l_p = g_reg()->ctx().at<project>().p_path / p_i->path;
    return l_p.lexically_normal();
  }
}
const FSys::path& assets_file::path_attr() const {
  return p_i->path;
}
void assets_file::path_attr(const FSys::path& in_path) {
  p_i->path = in_path;
}
void assets_file::name_attr(const std::string& in_name) const {
  p_i->p_name = in_name;
}
assets_file::assets_file(assets_file&&) noexcept            = default;
assets_file& assets_file::operator=(assets_file&&) noexcept = default;
assets_file::assets_file(const assets_file& in) noexcept
    : p_i(std::make_unique<impl>(*in.p_i)) {
}
assets_file& assets_file::operator=(const assets_file& in) noexcept {
  *p_i = *in.p_i;
  return *this;
}
const std::string& assets_file::organization_attr() const noexcept {
  return p_i->organization_p;
}
void assets_file::organization_attr(const std::string& in_organization) noexcept {
  p_i->organization_p = in_organization;
}
void assets_file::destruction_user(entt::registry& in_reg, entt::entity in_entt) {
  for (auto&& [e, ass] : in_reg.view<assets_file>().each()) {
    if (ass.p_i->handle_cache == in_entt) {
      ass.p_i->p_user.clear();
    }
  }
}
assets_file::~assets_file() = default;
}  // namespace doodle
