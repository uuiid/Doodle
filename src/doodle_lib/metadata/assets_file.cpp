//
// Created by TD on 2021/5/7.
//

#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/metadata/assets_path.h>
#include <doodle_lib/metadata/comment.h>
///这个工厂类必须在所有导入的后面
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <google/protobuf/util/time_util.h>
#include <metadata/time_point_wrap.h>
#include <pin_yin/convert.h>

namespace doodle {

assets_file::assets_file()
    : assets_file(std::string{}) {
}

assets_file::assets_file(std::string in_show_name)
    : p_name(convert::Get().toEn(in_show_name)),
      p_ShowName(in_show_name),
      p_user(core_set::getSet().get_user()),
      p_department(core_set::getSet().get_department_enum()),
      p_version(1) {}

std::string assets_file::str() const {
  return p_name;
}
std::string assets_file::show_str() const {
  return p_ShowName;
}

bool assets_file::operator<(const assets_file& in_rhs) const {
  // return std::tie(p_version, p_time->getUTCTime()) < std::tie(p_version, p_time->getUTCTime());
  return std::tie(p_version) < std::tie(in_rhs.p_version);
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

std::string assets_file::get_version_str() const {
  return fmt::format("v{:04d}", p_version);
}

void assets_file::set_version(const std::uint64_t& in_Version) noexcept {
  p_version = in_Version;
}

department assets_file::get_department() const {
  return p_department;
}
void assets_file::set_department(department in_department) {
  p_department = in_department;
}

void assets_file::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(this);
}
}  // namespace doodle
