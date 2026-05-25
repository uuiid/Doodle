#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>


namespace doodle::orm {
// 序列化 sql 上下文标准
class to_sql_ctx {
 public:
  enum to_sql_ctx_e {  // 这些上下文会影响 column_operations 中 operator=, operator== 等操作符生成的 SQL
                       // 片段中是否是bind参数，还是直接使用值
    select_sql              = 1 << 0,
    insert_sql              = 1 << 1,
    update_sql              = 1 << 2,
    delete_sql              = 1 << 3,

    // 这些上下文会传递给 column_operations 中的 to_sql 函数，以便生成不同的 SQL 片段
    create_trigger_sql      = 1 << 4,
    create_unique_index_sql = 1 << 5,
    create_index_sql        = 1 << 6,
    create_table_sql        = 1 << 7,
    // 别名上下文
    alias_sql               = 1 << 8,
    // where 条件上下文
    where_sql               = 1 << 9,
  };
  to_sql_ctx_e ctx_{select_sql};
  // namespace magic_enum::bitwise_operators;
  bool is_bind_param_{true};  // 是否生成 bind 参数，还是直接使用值
  // 枚举运算符重载
  to_sql_ctx& operator|(to_sql_ctx_e other) {
    ctx_ = static_cast<to_sql_ctx_e>(static_cast<int>(ctx_) | static_cast<int>(other));
    return *this;
  }

};

}  // namespace doodle::orm