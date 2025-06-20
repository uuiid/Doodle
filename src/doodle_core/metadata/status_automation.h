//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct task;
enum class status_automation_change_type { status, ready_for };
NLOHMANN_JSON_SERIALIZE_ENUM(
    status_automation_change_type,
    {
        {status_automation_change_type::status, "status"},
        {status_automation_change_type::ready_for, "ready_for"},
    }
)

struct status_automation {
  DOODLE_BASE_FIELDS();
  std::string entity_type_;
  uuid in_task_type_id_;
  uuid in_task_status_id_;
  status_automation_change_type out_field_type_;
  uuid out_task_type_id_;
  uuid out_task_status_id_;
  bool import_last_revision_;
  bool archived_;

  boost::asio::awaitable<void> run(const std::shared_ptr<task>& in_task, const uuid& in_person_id);

  // to json
  friend void to_json(nlohmann::json& j, const status_automation& p) {
    j["id"]                   = p.uuid_id_;
    j["entity_type"]          = p.entity_type_;
    j["in_task_type_id"]      = p.in_task_type_id_;
    j["in_task_status_id"]    = p.in_task_status_id_;
    j["out_field_type"]       = p.out_field_type_;
    j["out_task_type_id"]     = p.out_task_type_id_;
    j["out_task_status_id"]   = p.out_task_status_id_;
    j["import_last_revision"] = p.import_last_revision_;
    j["archived"]             = p.archived_;
  }
};
}  // namespace doodle