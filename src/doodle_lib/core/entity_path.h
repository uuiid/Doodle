#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <memory>

namespace doodle {

class entity_path_base {
 public:
  virtual ~entity_path_base()          = default;
  virtual FSys::path get_maya_path()   = 0;
  virtual FSys::path get_ue_path()     = 0;
  virtual FSys::path get_output_path() = 0;
};

std::shared_ptr<entity_path_base> create_entity_path(const uuid& in_task_id);

}  // namespace doodle