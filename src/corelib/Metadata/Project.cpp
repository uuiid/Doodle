// Fill out your copyright notice in the Description page of Project Settings.
#include <corelib/Metadata/Project.h>
#include <corelib/Exception/Exception.h>

#include <loggerlib/Logger.h>
#include <pinyinlib/convert.h>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace doodle {

Project::Project()
    : p_name(),
      p_path() {
}

Project::Project(FSys::path in_path, std::string in_name)
    : p_name(std::move(in_name)),
      p_path(std::move(in_path)) {
}

const std::string& Project::Name() const noexcept {
  return p_name;
}

void Project::setName(const std::string& Name) noexcept {
  p_path = Name;
}

const FSys::path& Project::Path() const noexcept {
  return p_path;
}

void Project::setPath(const FSys::path& Path) {
  if (Path.empty())
    throw DoodleError{"项目路径不能为空"};
  p_path = Path;
}

std::string Project::str() const {
  return boost::algorithm::to_lower_copy(
      dopinyin::convert::Get().toEn(this->p_name));
}

std::string Project::ShortStr() const {
  auto wstr       = boost::locale::conv::utf_to_utf<wchar_t>(this->p_name);
  auto& k_pingYin = dopinyin::convert::Get();
  std::string str{};
  for (auto s : wstr) {
    auto k_s_front = k_pingYin.toEn(s).front();
    str.append(&k_s_front, 1);
  }
  DOODLE_LOG_INFO(str);
  return boost::algorithm::to_upper_copy(str.substr(0, 2));
}

}  // namespace doodle