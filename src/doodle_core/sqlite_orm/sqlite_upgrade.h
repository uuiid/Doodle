//
// Created by TD on 25-5-15.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
struct sqlite_database_impl;
}
namespace doodle::details {

class sqlite_upgrade {
 public:
  virtual ~sqlite_upgrade()                                                  = default;
  virtual void upgrade(const std::shared_ptr<sqlite_database_impl>& in_data) = 0;
};

std::shared_ptr<sqlite_upgrade> upgrade_1(const FSys::path& in_db_path);

}  // namespace doodle::details