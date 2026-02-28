#include "comment.h"

#include <doodle_core/metadata/department.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::comment_ns {
void set_comment_mentions(comment& in_comment, const uuid& in_project_id) {
  for (auto&& person : g_ctx().get<sqlite_database>().get_project_persons(in_project_id)) {
    if (std::regex_search(in_comment.text_, std::regex(fmt::format("@{}( |$)", person.get_full_name())))) {
      in_comment.mentions_.emplace_back(person.uuid_id_);
    }
  }
}
void set_comment_department_mentions(comment& in_comment) {
  for (auto&& department : g_ctx().get<sqlite_database>().get_all<department>()) {
    if (std::regex_search(in_comment.text_, std::regex(fmt::format("@{}( |$)", department.name_)))) {
      in_comment.department_mentions_.emplace_back(department.uuid_id_);
    }
  }
}
}  // namespace doodle::comment_ns