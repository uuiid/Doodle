//
// Created by TD on 2021/5/7.
//

#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/uuid/uuid.hpp>

#include <core/core_set.h>
#include <entt/entity/fwd.hpp>
#include <functional>

namespace doodle {

class assets_file::impl {
 public:
  FSys::path path{};
  std::string p_name{};
  std::uint64_t p_version{};

  std::string organization_p{};
};

void to_json(nlohmann::json& j, const assets_file& p) {
  j["name"]           = p.p_i->p_name;
  j["organization_p"] = p.p_i->organization_p;
  j["version"]        = p.p_i->p_version;
  j["path"]           = p.p_i->path;
  j["user_ref2"]      = p.user_ref;
}
void from_json(const nlohmann::json& j, assets_file& p) {
  j.at("name").get_to(p.p_i->p_name);
  j.at("version").get_to(p.p_i->p_version);
  if (j.contains("organization_p")) j.at("organization_p").get_to(p.p_i->organization_p);
  if (j.contains("path")) j.at("path").get_to(p.p_i->path);
  if (j.contains("user_ref2")) {
    j.at("user_ref2").get_to(p.user_ref);
  } else {
    if (j.contains("user")) j.at("user").get_to(p.user_ref.cache_name);
    if (j.contains("user_ref")) p.user_ref.set_uuid(j.at("user_ref").at("uuid").get<boost::uuids::uuid>());
  }
}

assets_file::assets_file() : p_i(std::make_unique<impl>()){};

assets_file::assets_file(const FSys::path& in_path) : assets_file() {
  p_i->path   = in_path;
  p_i->p_name = in_path.stem().generic_string();
  user_attr(g_reg()->ctx().get<user::current_user>().get_handle());
  p_i->organization_p = core_set::get_set().organization_name;
}

assets_file::assets_file(const FSys::path& in_path, std::string in_name, std::uint64_t in_version) : assets_file() {
  p_i->path           = in_path;
  p_i->p_name         = std::move(in_name);
  p_i->p_version      = in_version;
  p_i->organization_p = core_set::get_set().organization_name;
  user_attr(g_reg()->ctx().get<user::current_user>().get_handle());
}

std::string assets_file::str() const { return p_i->p_name; }
const std::string& assets_file::name_attr() const { return p_i->p_name; }
bool assets_file::operator<(const assets_file& in_rhs) const {
  return std::tie(p_i->p_name, p_i->p_version) < std::tie(in_rhs.p_i->p_name, in_rhs.p_i->p_version);
}
bool assets_file::operator==(const assets_file& in_rhs) const {
  return std::tie(p_i->p_name, p_i->p_version) == std::tie(in_rhs.p_i->p_name, in_rhs.p_i->p_version);
}

entt::handle assets_file::user_attr() const { return user_ref.user_attr(); }
entt::handle assets_file::user_attr() { return user_ref.user_attr(); }
void assets_file::user_attr(const entt::handle& in_user) {
  DOODLE_CHICK(in_user.any_of<user>(), doodle_error{"句柄 {} 缺失必要组件 user", in_user});
  user_ref.user_attr(in_user);
}

const std::uint64_t& assets_file::version_attr() const noexcept { return p_i->p_version; }

void assets_file::version_attr(const std::uint64_t& in_Version) noexcept { p_i->p_version = in_Version; }
FSys::path assets_file::get_path_normal() const {
  DOODLE_CHICK(g_reg()->ctx().contains<project>(), doodle_error{"缺失项目上下文"});
  if (p_i->path.has_root_path())
    return p_i->path;
  else {
    auto l_p = g_reg()->ctx().get<project>().p_path / p_i->path;
    return l_p.lexically_normal();
  }
}
const FSys::path& assets_file::path_attr() const { return p_i->path; }
void assets_file::path_attr(const FSys::path& in_path) { p_i->path = in_path; }
void assets_file::name_attr(const std::string& in_name) const { p_i->p_name = in_name; }

assets_file::assets_file(assets_file&&) noexcept            = default;
assets_file& assets_file::operator=(assets_file&&) noexcept = default;

assets_file::assets_file(const assets_file& in) noexcept : p_i(std::make_unique<impl>(*in.p_i)) {
  user_ref = in.user_ref;
}
assets_file& assets_file::operator=(const assets_file& in) noexcept {
  *p_i     = *in.p_i;
  user_ref = in.user_ref;
  return *this;
}
const std::string& assets_file::organization_attr() const noexcept { return p_i->organization_p; }
void assets_file::organization_attr(const std::string& in_organization) noexcept {
  p_i->organization_p = in_organization;
}
assets_file::~assets_file() = default;

}  // namespace doodle
