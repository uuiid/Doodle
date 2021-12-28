//
// Created by TD on 2021/12/27.
//

#include "join_move.h"
namespace doodle {
namespace details {

class join_move::impl {
 public:
};

join_move::join_move(const entt::handle &in_handle, const std::vector<entt::handle> &in_vector)
    : p_i(std::make_unique<impl>()) {
}
void join_move::init() {
}
void join_move::succeeded() {
}
void join_move::failed() {
}
void join_move::aborted() {
}
void join_move::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
}
}  // namespace details
}  // namespace doodle
