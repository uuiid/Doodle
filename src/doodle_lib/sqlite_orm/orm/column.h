#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::orm {
class storage;
using storage_column_types =
    std::tuple<std::int64_t, std::double_t, std::string, uuid, chrono::system_zoned_time, nlohmann::json, FSys::path>;

// 将 storage_column_types 转换为 std::variant<std::int64_t, std::double_t, std::string, uuid,
// chrono::system_zoned_time, nlohmann::json, FSys::path>
template <typename Tuple>
struct tuple_to_variant;
template <typename... Ts>
struct tuple_to_variant<std::tuple<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename Tuple>
using tuple_to_variant_t     = typename tuple_to_variant<Tuple>::type;
using storage_column_variant = tuple_to_variant_t<storage_column_types>;

template <typename Table, typename Tuple>
struct tuple_to_table_member_variant;
template <typename Table, typename... Ts>
struct tuple_to_table_member_variant<Table, std::tuple<Ts...>> {
  using type = std::variant<Ts Table::*...>;
};

template <typename Table>
using table_columns_t = typename tuple_to_table_member_variant<Table, storage_column_types>::type;

// 运行时列信息
struct base_column_info_t {
  virtual ~base_column_info_t()                                                        = default;
  virtual std::string get_column_name(const storage& s, bool include_table_name) const = 0;
  virtual std::string get_table_name(const storage& s) const                           = 0;
};

// 基本的列信息，包含列名和成员指针
template <typename Table>
struct column_info_t : public base_column_info_t {
  table_columns_t<Table> ptr_;

  template <typename T>
  explicit column_info_t(auto T::* in_ptr) : ptr_(in_ptr) {}
  template <typename T>
  explicit column_info_t(const table_columns_t<T>& in_column) : ptr_(in_column) {}
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
};
using column_info_ptr = std::shared_ptr<base_column_info_t>;

}  // namespace doodle::orm