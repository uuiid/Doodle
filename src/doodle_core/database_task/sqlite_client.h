//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {
class sqlite_client {
 public:
  void open_sqlite(const FSys::path& in_path, bool only_ctx = false);
  void update_entt();
  void create_sqlite();
};

}  // namespace doodle::database_n
