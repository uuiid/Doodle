//
// Created by TD on 2022/4/29.
//

#include "parser_rpc.h"

#include <doodle_core/json_rpc/core/rpc_request.h>

#include <json_rpc/core/rpc_server.h>

namespace doodle::json_rpc {

std::string parser_rpc::operator()(const rpc_server& in_server) { return {}; }

void parser_rpc::json_data_attr(const std::string& in_string) { json_data_ = in_string; }
parser_rpc::operator bool() const { return is_close; }

rpc_request::identifier& rpc_request::identifier::get() {
  static identifier identifier1;
  return identifier1;
}
std::uint64_t rpc_request::identifier::id() { return ++id_; }
}  // namespace doodle::json_rpc
