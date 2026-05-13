#include "column_operations.h"

#include <fmt/format.h>

namespace doodle::orm {
std::string operator_compare_t::to_sql(const storage& s, bool include_table_name) const {
  return fmt::format(
      "({} {} {})", data_impl_ptr_->left_->to_sql(s, include_table_name), data_impl_ptr_->op_,
      data_impl_ptr_->right_->to_sql(s, include_table_name)
  );
}

void operator_compare_t::collect_bind_variants(
    std::vector<std::shared_ptr<storage_column_variant>>& bind_variants
) const {
  data_impl_ptr_->left_->collect_bind_variants(bind_variants);
  data_impl_ptr_->right_->collect_bind_variants(bind_variants);
}
}  // namespace doodle::orm