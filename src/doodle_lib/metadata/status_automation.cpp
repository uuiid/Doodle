#include "status_automation.h"

#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::status_automation_ns {

orm::sql_modify_statement_vector_t run(
    const status_automation& in_status_automation, const std::shared_ptr<task>& in_task, const uuid& in_person_id
) {
  if (in_status_automation.archived_ || in_task->task_type_id_ != in_status_automation.in_task_type_id_ ||
      in_task->task_status_id_ != in_status_automation.in_task_status_id_)
    return {};
  auto l_sql = get_sqlite_database();
  if (auto l_map = l_sql.get_task_type_priority_map(in_task->project_id_, in_status_automation.entity_type_);
      !l_map.empty() && in_status_automation.out_field_type_ != status_automation_change_type::ready_for &&
      ((l_map.contains(in_status_automation.in_task_type_id_) ? l_map.at(in_status_automation.in_task_type_id_) : 0) >
       (l_map.contains(in_status_automation.out_task_type_id_) ? l_map.at(in_status_automation.out_task_type_id_) : 0)))
    return {};
  orm::sql_modify_statement_vector_t l_sqls{};
  switch (in_status_automation.out_field_type_) {
    case status_automation_change_type::status:
      if (auto l_task =
              l_sql.get_tasks_for_entity_and_task_type(in_task->entity_id_, in_status_automation.out_task_type_id_);
          l_task) {
        auto l_task_type       = l_sql.get_by_uuid<task_type>(in_status_automation.in_task_type_id_);
        auto l_task_status     = l_sql.get_by_uuid<task_status>(in_status_automation.in_task_status_id_);
        auto l_task_out_status = l_sql.get_by_uuid<task_status>(in_status_automation.out_task_status_id_);
        auto l_comment         = std::make_shared<comment>(comment{
            .object_id_ = l_task->uuid_id_,
            .text_ = fmt::format("自动化任务 {} 更改触发, 设置状态 {} ", l_task_type.name_, l_task_out_status.name_),
            .task_status_id_ = l_task_status.uuid_id_,
            .person_id_      = in_person_id,
        });

        using namespace orm;
        l_sqls.emplace_back(insert(l_sql).into<comment>().values(*l_comment));
        l_task->task_status_id_     = l_task_out_status.uuid_id_;
        l_task->updated_at_         = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
        in_task->task_status_id_    = l_task_status.uuid_id_;
        in_task->last_comment_date_ = l_comment->created_at_;
        in_task->updated_at_        = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
        l_sqls.emplace_back(update(l_sql)
                                .from<task>()
                                .set(c(&task::task_status_id_) = l_task->task_status_id_)
                                .set(c(&task::updated_at_) = l_task->updated_at_)
                                .where(c(&task::uuid_id_) == l_task->uuid_id_));
        l_sqls.emplace_back(update(l_sql)
                                .from<task>()
                                .set(c(&task::task_status_id_) = in_task->task_status_id_)
                                .set(c(&task::last_comment_date_) = in_task->last_comment_date_)
                                .set(c(&task::updated_at_) = in_task->updated_at_)
                                .where(c(&task::uuid_id_) == in_task->uuid_id_));
      }

      break;
    case status_automation_change_type::ready_for: {
      auto l_entt = l_sql.get_by_uuid<entity>(in_task->entity_id_);
      if (l_entt.ready_for_ == in_status_automation.out_task_type_id_) return l_sqls;
      using namespace orm;
      l_sqls.emplace_back(update(l_sql)
                              .from<entity>()
                              .set(c(&entity::ready_for_) = in_status_automation.out_task_type_id_)
                              .where(c(&entity::uuid_id_) == l_entt.uuid_id_));
    } break;
  }

  return l_sqls;
}

}  // namespace doodle::status_automation_ns