// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

#include <core/core_set.h>
#include <exception/exception.h>

namespace doodle {

project::project() : p_name("none"), p_path("C:/") {}

project::project(FSys::path in_path, std::string in_name) : project() {
  p_name = std::move(in_name);
  p_path = std::move(in_path);
}
project::project(const project_helper::database_t& in)
    : p_name(in.name_),
      p_path(in.path_),
      p_en_str(in.en_str_),
      p_shor_str(in.shor_str_),
      p_local_path(in.local_path_),
      p_auto_upload_path(in.auto_upload_path_) {}

project::operator project_helper::database_t() const {
  project_helper::database_t l_ret{
      .id_               = 0,
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = p_name,
      .path_             = p_path.generic_string(),
      .en_str_           = p_en_str,
      .shor_str_         = p_shor_str,
      .local_path_       = p_local_path.generic_string(),
      .auto_upload_path_ = p_auto_upload_path.generic_string()
  };
  return l_ret;
}

void project::set_name(const std::string& Name) noexcept {
  if (Name == p_name) return;
  p_name = Name;
}

const FSys::path& project::get_path() const noexcept { return p_path; }

void project::set_path(const FSys::path& Path) {
  //  DOODLE_CHICK(!Path.empty(), doodle_error{"项目路径不能为空"});
  if (p_path == Path) return;

  p_path = Path;
}

std::string project::str() const { return p_en_str; }

std::string project::short_str() const { return p_shor_str; }

std::string project::show_str() const { return this->p_name; }

bool project::operator<(const project& in_rhs) const {
  return std::tie(p_name, p_en_str, p_shor_str, p_path) <
         std::tie(in_rhs.p_name, in_rhs.p_en_str, in_rhs.p_shor_str, in_rhs.p_path);
}
bool project::operator==(const project& in_rhs) const {
  return std::tie(p_name, p_en_str, p_shor_str, p_path) ==
         std::tie(in_rhs.p_name, in_rhs.p_en_str, in_rhs.p_shor_str, in_rhs.p_path);
}

const std::string& project::get_name() const { return p_name; }

FSys::path project::make_path(const FSys::path& in_path) const {
  auto path = p_path / in_path;
  if (!exists(path)) create_directories(path);
  return path;
}

}  // namespace doodle
