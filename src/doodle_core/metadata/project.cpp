// Fill out your copyright notice in the Description page of Project Settings.
#include <exception/exception.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/pin_yin/convert.h>
#include <doodle_core/metadata/metadata.h>
#include <core/core_set.h>
#include "organization.h"
#include <doodle_core/logger/logger.h>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace doodle {

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
  DOODLE_CHICK(!Path.empty(), doodle_error{"项目路径不能为空"});
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

void project::init_name() {
  p_en_str = boost::algorithm::to_lower_copy(
      convert::Get().toEn(this->p_name)
  );
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

FSys::path project::make_path(const FSys::path& in_path) const {
  auto path = p_path / in_path;
  if (!exists(path))
    create_directories(path);
  return path;
}

project_config::base_config::base_config()
    : vfx_cloth_sim_path(),
      export_group("UE4"),
      cloth_proxy_("_cloth_proxy"),
      simple_module_proxy_("_proxy"),
      find_icon_regex(),
      assets_list(),
      icon_extensions({".png"s, ".jpg"s}),
      season_count(20) {}

void project_config::to_json(nlohmann::json& j, const base_config& p) {
  j["find_icon_regex"]      = p.find_icon_regex;
  j["assets_list"]          = p.assets_list;
  j["vfx_cloth_sim_path"]   = p.vfx_cloth_sim_path;
  j["export_group"]         = p.export_group;
  j["cloth_proxy_"]         = p.cloth_proxy_;
  j["simple_module_proxy_"] = p.simple_module_proxy_;
  j["icon_extensions"]      = p.icon_extensions;
  j["upload_path"]          = p.upload_path;
  j["season_count"]         = p.season_count;
}
void project_config::from_json(const nlohmann::json& j, base_config& p) {
  if (j.contains("find_icon_regex"))
    j.at("find_icon_regex").get_to(p.find_icon_regex);
  if (j.contains("assets_list"))
    j.at("assets_list").get_to(p.assets_list);
  j.at("vfx_cloth_sim_path").get_to(p.vfx_cloth_sim_path);
  if (j.contains("export_group"))
    j.at("export_group").get_to(p.export_group);
  if (j.contains("cloth_proxy_"))
    j.at("cloth_proxy_").get_to(p.cloth_proxy_);
  if (j.contains("simple_module_proxy_"))
    j.at("simple_module_proxy_").get_to(p.simple_module_proxy_);
  if (j.contains("icon_extensions"))
    j.at("icon_extensions").get_to(p.icon_extensions);
  if (j.contains("upload_path"))
    j.at("upload_path").get_to(p.upload_path);
  if (j.contains("season_count"))
    j.at("season_count").get_to(p.season_count);
}

bool project_config::base_config::match_icon_extensions(const FSys::path& in_path) const {
  auto&& l_p = in_path.extension();
  for (auto&& i : icon_extensions) {
    if (l_p == i)
      return true;
  }
  return false;
}
FSys::path project_config::base_config::get_upload_path() const {
  if (upload_path.has_root_path())
    return upload_path;
  else
    return g_reg()->ctx().at<project>().p_path / upload_path;
}
}  // namespace doodle
