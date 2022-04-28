//
// Created by TD on 2021/5/7.
//

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>


#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/pin_yin/convert.h>
#include <core/core_set.h>
#include <metadata/project.h>

namespace doodle {

assets_file::assets_file() = default;

assets_file::assets_file(const FSys::path& in_path)
    : assets_file(in_path, in_path.stem().generic_string(), 0) {
}

assets_file::assets_file(const FSys::path& in_path,
                         std::string in_name,
                         std::uint64_t in_version)
    : path(in_path),
      p_name(std::move(in_name)),
      p_version(in_version),
      p_user(core_set::getSet().get_user()),
      organization_p(core_set::getSet().organization_name) {
}

std::string assets_file::str() const {
  return p_name;
}

bool assets_file::operator<(const assets_file& in_rhs) const {
  // return std::tie(p_version, p_time->getUTCTime()) < std::tie(p_version, p_time->getUTCTime());
  return std::tie(p_name, p_version) < std::tie(in_rhs.p_name, in_rhs.p_version);
  //  return std::tie(p_name,p_version) < std::tie(p_name,in_rhs.p_version);
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name, p_ShowName) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_ShowName);
}
bool assets_file::operator>(const assets_file& in_rhs) const {
  return in_rhs < *this;
}
bool assets_file::operator<=(const assets_file& in_rhs) const {
  return !(in_rhs < *this);
}
bool assets_file::operator>=(const assets_file& in_rhs) const {
  return !(*this < in_rhs);
}

const std::string& assets_file::get_user() const {
  return p_user;
}
void assets_file::set_user(const std::string& in_user) {
  p_user = in_user;
}

const std::uint64_t& assets_file::get_version() const noexcept {
  return p_version;
}

void assets_file::set_version(const std::uint64_t& in_Version) noexcept {
  p_version = in_Version;
}
FSys::path assets_file::get_path_normal() const {
  chick_true<doodle_error>(g_reg()->ctx().contains<project>(), DOODLE_LOC, "缺失项目上下文");
  auto l_p = g_reg()->ctx().at<project>().p_path / path;
  return l_p.lexically_normal();
}

}  // namespace doodle
