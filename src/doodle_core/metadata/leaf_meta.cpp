#include "leaf_meta.h"

namespace doodle {
ref_meta::ref_meta()
    : ref_list() {
}
void to_json(nlohmann::json& j, const ref_meta& p) {
  j["ref_meta"] = p.ref_list;
}
void from_json(const nlohmann::json& j, ref_meta& p) {
  j["ref_meta"].get_to(p.ref_list);
}
}  // namespace doodle
