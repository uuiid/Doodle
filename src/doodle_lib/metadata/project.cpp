// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <exception/exception.h>
#include <metadata/metadata_factory.h>
#include <metadata/project.h>
#include <pin_yin/convert.h>

#include <boost/locale.hpp>

namespace doodle {
project::cloth_config::cloth_config()
    : vfx_cloth_sim_path("C:/"),
      simple_subsampling(true),
      frame_samples(10),
      time_scale(0.1),
      length_scale(0.4),
      cloth_shape("_clothShape"),
      cloth_proxy("_cloth_proxy"),
      export_group("UE4") {}

project::project()
    : p_name("none"),
      p_path("C:/"),
      p_en_str(),
      p_shor_str() {
}

project::project(FSys::path in_path, std::string in_name)
    : project() {
  p_name = std::move(in_name);
  p_path = std::move(in_path);
  init_name();
}

void project::set_name(const std::string& Name) noexcept {
  if (Name == p_name)
    return;
  p_name = Name;
  init_name();
}

const FSys::path& project::get_path() const noexcept {
  return p_path;
}

void project::set_path(const FSys::path& Path) {
  chick_true<doodle_error>(!Path.empty(), DOODLE_LOC, "项目路径不能为空");
  if (p_path == Path)
    return;

  p_path = Path;
}

std::string project::str() const {
  return p_en_str;
}

std::string project::short_str() const {
  return p_shor_str;
}

std::string project::show_str() const {
  return this->p_name;
}

bool project::operator<(const project& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name, p_path) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_path);
  return std::tie(p_name, p_path) < std::tie(in_rhs.p_name, in_rhs.p_path);
}
bool project::operator>(const project& in_rhs) const {
  return in_rhs < *this;
}
bool project::operator<=(const project& in_rhs) const {
  return !(in_rhs < *this);
}
bool project::operator>=(const project& in_rhs) const {
  return !(*this < in_rhs);
}

const std::string& project::get_name() const {
  return p_name;
}

void project::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(this);
}
void project::init_name() {
  p_en_str = boost::algorithm::to_lower_copy(
      convert::Get().toEn(this->p_name));
  auto wstr       = boost::locale::conv::utf_to_utf<wchar_t>(this->p_name);
  auto& k_pingYin = convert::Get();
  std::string str{};
  for (auto s : wstr) {
    auto k_s_front = k_pingYin.toEn(s).front();
    str.append(&k_s_front, 1);
  }
  DOODLE_LOG_INFO(str)
  p_shor_str = boost::algorithm::to_upper_copy(str.substr(0, 2));
}
bool project::operator==(const project& in_rhs) const {
  return std::tie(p_name, p_en_str, p_shor_str, p_path) == std::tie(in_rhs.p_name, in_rhs.p_en_str, in_rhs.p_shor_str, in_rhs.p_path);
}
bool project::operator!=(const project& in_rhs) const {
  return !(in_rhs == *this);
}

project::cloth_config& project::get_vfx_cloth_config() const {
  auto k_h = make_handle(*this);
  return k_h.get_or_emplace<cloth_config>();
}

}  // namespace doodle
