// Fill out your copyright notice in the Description page of Project Settings.
#include <corelib/Metadata/Project.h>
#include <corelib/Exception/Exception.h>

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

}  // namespace doodle