#pragma once
#include <sqlite_orm/sqlite_orm.h>

namespace doodle {
class storage {
  struct column_info {
    std::string name_;
    std::string type_;
    bool not_null_{};
    bool primary_key_{};
    bool autoincrement_{};
  };

  struct table_info {
    std::string name_;
    std::vector<column_info> columns_;
  };
  std::vector<table_info> tables_;

 public:
  virtual ~storage() = default;

  template <typename T>
  storage& reg(T&& in_table) {
    // 解析 make_table 的返回值, 获取表名和列信息

    

    return *this;
  }
};
}  // namespace doodle