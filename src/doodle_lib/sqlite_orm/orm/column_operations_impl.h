#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>


namespace doodle::orm {
template <typename T>
column_operations::column_operations(auto T::* in_ptr) : data_impl_ptr_(std::make_shared<data_impl>()) {
  data_impl_ptr_->ptr_shared_ = std::make_shared<column_info_t>(in_ptr);
};

}  // namespace doodle::orm