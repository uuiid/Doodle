#include "doodle_core/metadata/work_task.h"

namespace doodle {

void to_json(nlohmann::json& j, const work_task_info& p) {
  j["time"]      = p.time;
  j["task_name"] = p.task_name;
  j["region"]    = p.region;
  j["abstract"]  = p.abstract;
  j["user"]      = p.user_ref;
}
void from_json(const nlohmann::json& j, work_task_info& p) {
  j.at("time").get_to(p.time);
  j.at("task_name").get_to(p.task_name);
  j.at("region").get_to(p.region);
  j.at("abstract").get_to(p.abstract);
  j.at("user").get_to(p.user_ref);
}

}  // namespace doodle