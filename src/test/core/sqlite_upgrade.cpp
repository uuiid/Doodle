//
// 数据库版本升级测试.
//
// 升级器的结构是"一步一个版本": upgrade_N_t 各自判断自己该不该跑.
// 本轮之前链条是 29 -> 30 -> 31; 29 -> 30 已随该步骤进入生产而移除 (见 sqlite_upgrade.cpp 顶部注释),
// 所以现在**只剩 30 -> 31 一步** (生产库停在 30), 升级链也只认 30 及以上的库.
// 这种结构下最危险的错误是"跳过"—— 某个步骤没执行, 但 user_version 被写成了最新版,
// 于是库看起来是新的, 实际迁移根本没做. 这里的用例专门盯住这一点.
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/scope/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_upgrade)


BOOST_AUTO_TEST_SUITE_END()
