#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/session.h>

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::orm {
namespace detail {
template <typename T>
struct update_arg_type {
  using type = std::decay_t<T>;
};

template <typename C, typename T>
struct update_arg_type<T C::*> {
  using type = std::decay_t<T>;
};

template <typename Table>
struct update_arg_type<object_t<Table>> {
  using type = Table;
};

template <typename T>
using update_arg_type_t = typename update_arg_type<std::decay_t<T>>::type;

}  // namespace detail

struct DOODLELIB_API update_t : public statement_info_base_t {
 private:
  struct update_state_t {
    std::vector<std::shared_ptr<column_operations_base_t>> column_operations_;
    std::string from_table_name_;
    std::shared_ptr<column_operations_base_t> wheres_;
    session s_{};
    std::shared_ptr<sqlite_stmt> stmt_;
    bind_value_collector_t bind_variants_{};
  };

  friend update_t update(const session& s);

  std::shared_ptr<update_state_t> state_;
  /// 测试成员字段 updated_at_ 是否存在，以及是否是 chrono::system_zoned_time 类型
  template <typename T, typename = void>
  struct has_updated_at_impl : std::false_type {};

  template <typename T>
  struct has_updated_at_impl<
      T, std::enable_if_t<std::is_same_v<chrono::system_zoned_time, decltype(std::declval<T>().updated_at_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_updated_at = has_updated_at_impl<T>::value;

 public:
  update_t() : state_(std::make_shared<update_state_t>()) {}

  template <typename T>
  update_t where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    state_->wheres_          = l_condition_fun_ptr;
    return *this;
  }
  template <typename FromTable>
  update_t from() {
    state_->from_table_name_ = state_->s_.get_table_name<FromTable>();
    return *this;
  }

  template <typename... TableColumns>
    requires((std::is_base_of_v<column_operations, std::decay_t<TableColumns>> && ...))
  update_t set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      auto col_ptr = std::make_shared<std::decay_t<decltype(in_column)>>(std::forward<decltype(in_column)>(in_column));
      state_->column_operations_.push_back(col_ptr);
    };
    (l_iter_fun(in_columns), ...);
    return *this;
  }
  /// 从 json 中提取属性列表声明的字段, 按类型检查后生成 SET 表达式
  template <typename Tuple>
  update_t set_from_ref(const nlohmann::json& in_json, const Tuple& in_property_list) {
    return set_from_ref_impl(in_json, in_property_list, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  }
  template <typename T>
  update_t set_from_ref(const nlohmann::json& in_json) {
    from<T>();
    if constexpr (has_updated_at<T>) {
      column_operations l_col{&T::updated_at_};
      l_col = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};;
      this->set(std::move(l_col));
    }
    return set_from_ref_impl(
        in_json, T::put_property_list(), std::make_index_sequence<std::tuple_size_v<decltype(T::put_property_list())>>{}
    );
  }

  template <typename T>
  update_t set_value(T&& in_object) {
    using Table = std::decay_t<T>;
    if constexpr (has_updated_at<T>)
      in_object.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    auto l_table_cloums = state_->s_.template get_table_columns<Table>();
    column_info l_primary_key_{};
    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) {  // 主键不更新
        l_primary_key_ = l_column;
        continue;
      }
      auto col_ptr = std::make_shared<column_operations>(l_column.ptr_);
      *col_ptr     = l_column.ptr_.get_value(in_object);

      state_->column_operations_.push_back(col_ptr);
    }
    from<Table>();
    where(column_operations{l_primary_key_.ptr_} == l_primary_key_.ptr_.get_value(in_object));
    return *this;
  }

  template <typename T>
  update_t rebind(T&& in_object) {
    using Table = std::decay_t<T>;
    if constexpr (has_updated_at<T>)
      in_object.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    auto l_table_cloums = state_->s_.template get_table_columns<Table>();
    if (l_table_cloums.size() != state_->bind_variants_.bind_values_.size())
      throw std::runtime_error("列数量与绑定变量数量不匹配，无法使用 re_set 更新");

    column_info l_primary_key_{};
    for (auto l_i = 0; const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) {  // 主键不更新
        l_primary_key_ = l_column;
        continue;
      }
      auto col_ptr                               = std::make_shared<column_operations>(l_column.ptr_);
      state_->bind_variants_.bind_values_[l_i++] = l_column.ptr_.get_value(in_object);
    }
    state_->bind_variants_.bind_values_.back() = l_primary_key_.ptr_.get_value(in_object);
    return *this;
  }

  void prepare(session& s, const to_sql_ctx& ctx) override;

  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;

  update_t operator()();

 private:
  /// 按成员指针类型检查 json 值, 通过后写入 SET 表达式
  template <typename MemberPtr>
  static void assign_json_value(column_operations& in_col, const nlohmann::json& in_json) {
    using value_t = class_attr_type_t<std::decay_t<MemberPtr>>;
    in_col        = in_json.get<value_t>();
    // return true;
  }

  template <typename Pair>
  void apply_json_property(const nlohmann::json& in_json, const Pair& in_pair) {
    if (!in_json.contains(in_pair.first)) return;
    column_operations l_col{in_pair.second};
    assign_json_value<decltype(in_pair.second)>(l_col, in_json.at(in_pair.first));
    this->set(std::move(l_col));
  }

  template <typename Tuple, std::size_t... I>
  update_t set_from_ref_impl(const nlohmann::json& in_json, const Tuple& in_property_list, std::index_sequence<I...>) {
    (apply_json_property(in_json, std::get<I>(in_property_list)), ...);
    return *this;
  }
};

inline update_t update(const session& s) {
  update_t l_update{};
  l_update.state_->s_ = s;
  return l_update;
}
}  // namespace doodle::orm