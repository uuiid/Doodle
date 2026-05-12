#include "column_operations.h"

#include <fmt/format.h>

namespace doodle::orm {
std::string operator_compare_t::to_sql(const storage& s, bool include_table_name) const {
  return fmt::format(
      "({} {} {})", data_impl_ptr_->left_->to_sql(s, include_table_name), data_impl_ptr_->op_,
      data_impl_ptr_->right_->to_sql(s, include_table_name)
  );
}

const std::vector<std::shared_ptr<storage_column_variant>>& operator_compare_t::get_value_variants() const {
  if (data_impl_ptr_->cached_variants_.empty()) {
    const auto& left_variants  = data_impl_ptr_->left_->get_value_variants();
    const auto& right_variants = data_impl_ptr_->right_->get_value_variants();
    data_impl_ptr_->cached_variants_.insert(
        data_impl_ptr_->cached_variants_.end(), left_variants.begin(), left_variants.end()
    );
    data_impl_ptr_->cached_variants_.insert(
        data_impl_ptr_->cached_variants_.end(), right_variants.begin(), right_variants.end()
    );
  }
  return data_impl_ptr_->cached_variants_;
}
}  // namespace doodle::orm