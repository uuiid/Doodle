//
// Created by TD on 2022/5/17.
//

#include "json_rpc_process_t.h"
#include <json_rpc/core/server.h>
#include <doodle_core/core/doodle_lib.h>
#include <boost/asio.hpp>
namespace doodle {
class json_rpc_process_t::impl {
 public:
  explicit impl(std::uint16_t in_port) {}
};

json_rpc_process_t::json_rpc_process_t(std::uint16_t in_port)
    : ptr(std::make_unique<impl>(in_port)) {
}
void json_rpc_process_t::init() {
}
void json_rpc_process_t::succeeded() {
}
void json_rpc_process_t::failed() {
}
void json_rpc_process_t::aborted() {
}
void json_rpc_process_t::update(
    const chrono::system_clock::duration& in_duration,
    void* in_data
) {
}
json_rpc_process_t::~json_rpc_process_t() = default;

}  // namespace doodle
