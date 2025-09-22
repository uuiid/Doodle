//
// Created by TD on 25-4-30.
//
#include "status_automation.h"

#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include "core/chrono_.h"
#include "sqlite_orm/sqlite_database.h"
#include <chrono>

namespace doodle {
boost::asio::awaitable<void> status_automation::run(const std::shared_ptr<task>& in_task, const uuid& in_person_id) {
  if (!archived_ || in_task->task_type_id_ != in_task_type_id_ || in_task->task_status_id_ != in_task_status_id_)
    co_return;
  auto l_sql = g_ctx().get<sqlite_database>();
  if (auto l_map = l_sql.get_task_type_priority_map(in_task->project_id_, entity_type_);
      !l_map.empty() && out_field_type_ != status_automation_change_type::ready_for &&
      ((l_map.contains(in_task_type_id_) ? l_map.at(in_task_type_id_) : 0) >
       (l_map.contains(out_task_type_id_) ? l_map.at(out_task_type_id_) : 0)))
    co_return;
  switch (out_field_type_) {
    case status_automation_change_type::status:
      if (auto l_task = l_sql.get_tasks_for_entity_and_task_type(in_task->entity_id_, out_task_type_id_); l_task) {
        auto l_task_type            = l_sql.get_by_uuid<task_type>(in_task_type_id_);
        auto l_task_status          = l_sql.get_by_uuid<task_status>(in_task_status_id_);
        auto l_comment              = std::make_shared<comment>(comment{
                         .uuid_id_    = core_set::get_set().get_uuid(),
                         .object_id_  = in_task->uuid_id_,
                         .text_       = fmt::format("自动化任务 {}更改触发, 设置状态{} ", l_task_type.name_, l_task_status.name_),
                         .created_at_ = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()},
                         .updated_at_ = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()},
                         .task_status_id_ = l_task_status.uuid_id_,
                         .person_id_      = in_person_id,
        });
        in_task->task_status_id_    = l_task_status.uuid_id_;
        in_task->last_comment_date_ = l_comment->created_at_;
        in_task->updated_at_        = l_comment->updated_at_;
        co_await l_sql.install(l_comment);
        co_await l_sql.install(in_task);
      }

      break;
    case status_automation_change_type::ready_for: {
      auto l_entt = std::make_shared<entity>(l_sql.get_by_uuid<entity>(in_task->entity_id_));
      if (l_entt->ready_for_ == out_task_type_id_) co_return;
      l_entt->ready_for_ = out_task_type_id_;
      co_await l_sql.install(l_entt);
    } break;
  }
}

}  // namespace doodle