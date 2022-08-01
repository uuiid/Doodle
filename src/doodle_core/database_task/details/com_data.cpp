//
// Created by TD on 2022/6/1.
//

#include "com_data.h"

namespace doodle {
namespace database_n {
namespace details {
bool com_data::operator<(const com_data& in_rhs) const {
  return std::tie(entt_, com_id) < std::tie(in_rhs.entt_, in_rhs.com_id);
}
bool com_data::operator>(const com_data& in_rhs) const {
  return in_rhs < *this;
}
bool com_data::operator<=(const com_data& in_rhs) const {
  return !(in_rhs < *this);
}
bool com_data::operator>=(const com_data& in_rhs) const {
  return !(*this < in_rhs);
}
}  // namespace details
}  // namespace database_n
}  // namespace doodle
