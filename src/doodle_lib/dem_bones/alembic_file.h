#include "doodle_lib/doodle_lib_fwd.h"

#include <memory>
#include <string>

namespace doodle::dem_bones {

class alembic_file {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  alembic_file();
  ~alembic_file();

  void open(const std::string& in_name);
};

}  // namespace doodle::dem_bones