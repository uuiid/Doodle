//
// Created by TD on 2022/9/30.
//

#include "json_rpc_facet.h"

namespace doodle {
namespace facet {

class json_rpc_facet::impl {
 public:
  std::string name_attr{"json_rpc"};
};

json_rpc_facet::json_rpc_facet()
    : p_i(std::make_unique<impl>()) {}

const std::string& json_rpc_facet::name() const noexcept {
  return p_i->name_attr;
}
void json_rpc_facet::operator()() {
}
void json_rpc_facet::deconstruction() {
}

json_rpc_facet::~json_rpc_facet() =default;
}  // namespace facet

}  // namespace doodle
