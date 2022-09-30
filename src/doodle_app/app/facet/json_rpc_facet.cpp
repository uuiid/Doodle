//
// Created by TD on 2022/9/30.
//

#include "json_rpc_facet.h"

namespace doodle {
namespace facet {
const std::string& json_rpc_facet::name() const noexcept {
  return name_attr;
}
void json_rpc_facet::operator()() {
}
void json_rpc_facet::deconstruction() {
}
json_rpc_facet::json_rpc_facet()
    : name_attr("json_rpc") {}
}  // namespace facet


}  // namespace doodle
