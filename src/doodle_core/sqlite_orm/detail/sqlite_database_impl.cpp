//
// Created by TD on 25-2-20.
//

#include "sqlite_database_impl.h"

#include <sqlite_orm/sqlite_orm.h>
#include <type_traits>
namespace doodle::details {
sqlite_orm_type make_storage_doodle_impl(const std::string& in_path) { return make_storage_doodle(in_path); }

}  // namespace doodle::details