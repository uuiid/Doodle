// Fill out your copyright notice in the Description page of Project Settings.
#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <Exception/Exception.h>
#include <Metadata/MetadataFactory.h>
#include <Metadata/Project.h>
#include <PinYin/convert.h>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::Project)
namespace doodle {

Project::Project()
    : p_name("none"),
      p_path("C:/"),
      p_en_str(),
      p_shor_str() {
  p_type = meta_type::project_root;
}

Project::Project(FSys::path in_path, std::string in_name)
    : p_name(std::move(in_name)),
      p_path(std::move(in_path)) {
  p_type = meta_type::project_root;
}

void Project::setName(const std::string& Name) noexcept {
  if (Name == p_name)
    return;
  p_name = Name;
  init();
  saved(true);
}

const FSys::path& Project::getPath() const noexcept {
  return p_path;
}

void Project::setPath(const FSys::path& Path) {
  if (Path.empty())
    throw DoodleError{"项目路径不能为空"};
  if (p_path == Path)
    return;

  p_path = Path;
  saved(true);
}

std::string Project::str() const {
  return p_en_str;
}

std::string Project::shortStr() const {
  return p_shor_str;
}

std::string Project::showStr() const {
  return this->p_name;
}
std::string Project::getConfigFileFolder() {
  static std::string str{".doodle_config"};
  return str;
}

std::string Project::getConfigFileName() {
  static std::string str{"doodle_config.dole"};
  return str;
}
FSys::path Project::DBRoot() const {
  return p_path / "_._root";
}

bool Project::operator<(const Project& in_rhs) const {
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name, p_path) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_path);
  return std::tie(p_name, p_path) < std::tie(in_rhs.p_name, in_rhs.p_path);
}
bool Project::operator>(const Project& in_rhs) const {
  return in_rhs < *this;
}
bool Project::operator<=(const Project& in_rhs) const {
  return !(in_rhs < *this);
}
bool Project::operator>=(const Project& in_rhs) const {
  return !(*this < in_rhs);
}

const std::string& Project::getName() const {
  return p_name;
}

void Project::create_menu(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<Project>(shared_from_this()));
}
void Project::init() {
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
