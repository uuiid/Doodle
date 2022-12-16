#include "doodle_core/doodle_core_fwd.h"

#include "metadata/time_point_wrap.h"
#include <string>

namespace doodle {
class DOODLE_CORE_API work_task_info {
 public:
  time_point_wrap time;
  std::string task_name;
  std::string region;
  std::string abstract;

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const work_task_info& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, work_task_info& p);
};
}  // namespace doodle