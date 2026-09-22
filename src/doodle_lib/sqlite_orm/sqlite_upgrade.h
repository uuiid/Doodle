//
// Created by TD on 25-5-15.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class sqlite_storage;
}
namespace doodle::details {

class sqlite_upgrade {
 public:
  virtual ~sqlite_upgrade()                     = default;
  virtual void upgrade(sqlite_storage& in_data) = 0;
};

std::shared_ptr<sqlite_upgrade> upgrade_init();
std::shared_ptr<sqlite_upgrade> upgrade_2();
// 非版本迁移: 不看 user_version、也不写 user_version, 每次升级都执行一次(启动清理)
std::shared_ptr<sqlite_upgrade> upgrade_clear_orphan_task();

}  // namespace doodle::details