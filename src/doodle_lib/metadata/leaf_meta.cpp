#include "leaf_meta.h"

namespace doodle {
void leaf_meta::set_metadata(const std::weak_ptr<metadata>& in_meta) {
  p_meta = in_meta;
}
std::weak_ptr<metadata> leaf_meta::get_metadata() const {
  return p_meta;
}
}  // namespace doodle