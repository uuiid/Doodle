// Fill out your copyright notice in the Description page of Project Settings.
#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <Exception/exception.h>
#include <Metadata/metadata_factory.h>
#include <Metadata/project.h>
#include <PinYin/convert.h>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::project)
namespace doodle {

project::project()
    : p_name("none"),
      p_path("C:/"),
      p_en_str(),
      p_shor_str() {
  p_type = meta_type::project_root;
}

project::project(FSys::path in_path, std::string in_name)
    : p_name(std::move(in_name)),
      p_path(std::move(in_path)) {
  p_type = meta_type::project_root;
}

void project::setName(const std::string& Name) noexcept {
  if (Name == p_name)
    return;
  p_name = Name;
  init();
  saved(true);
}

const FSys::path& project::getPath() const noexcept {
  return p_path;
}

void project::setPath(const FSys::path& Path) {
  if (Path.empty())
    throw DoodleError{"项目路径不能为空"};
  if (p_path == Path)
    return;

  p_path = Path;
  saved(true);
}

std::string project::str() const {
  return p_en_str;
}

std::string project::shortStr() const {
  return p_shor_str;
}

std::string project::showStr() const {
  return this->p_name;
}
std::string project::getConfigFileFolder() {
  static std::string str{".doodle_config"};
  return str;
}

std::string project::getConfigFileName() {
  static std::string str{"doodle_config.dole"};
  return str;
}
FSys::path project::DBRoot() const {
  return p_path / "_._root";
}

bool project::operator<(const project& in_rhs) const {
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name, p_path) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_path);
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

const std::string& project::getName() const {
  return p_name;
}

void project::create_menu(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<project>(shared_from_this()));
}
void project::init() {
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

}  // namespace doodle
