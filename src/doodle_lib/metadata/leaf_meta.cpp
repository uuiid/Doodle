#include "leaf_meta.h"

namespace doodle {
void leaf_meta::set_metadata(const std::weak_ptr<metadata>& in_meta) {
  p_meta = in_meta;
}
}  // namespace doodle