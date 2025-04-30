//
// Created by TD on 2021/5/18.
//

#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <core/core_set.h>
namespace doodle {

void comment::set_comment_mentions(const uuid& in_project_id) {
  for (auto&& person : g_ctx().get<sqlite_database>().get_project_persons(in_project_id)) {
    if (std::regex_search(text_, std::regex(fmt::format("@{}( |$)", person.get_full_name())))) {
      mentions_.emplace_back(person.uuid_id_);
    }
  }
}
void comment::set_comment_department_mentions() {
  for (auto&& department : g_ctx().get<sqlite_database>().get_all<department>()) {
    if (std::regex_search(text_, std::regex(fmt::format("@{}( |$)", department.name_)))) {
      department_mentions_.emplace_back(department.uuid_id_);
    }
  }
}

}  // namespace doodle
