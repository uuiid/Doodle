//
// ORM 表结构 (schema) 断言.
// 这些用例只需要一个空库, 用于锁定 regs_all() 里声明的表结构, 不需要真实库.
//

#include <doodle_core/metadata/comment.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_schema)

BOOST_AUTO_TEST_SUITE_END()
