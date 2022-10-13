//
// Created by TD on 2022/10/13.
//

#include "null_facet.h"

namespace doodle {
namespace maya_plug {
const std::string& null_facet::name() const noexcept {
  static std::string l_i{"null_facet"};
  return l_i;
}
}  // namespace maya_plug
}  // namespace doodle