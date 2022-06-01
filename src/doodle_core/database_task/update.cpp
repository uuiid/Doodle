//
// Created by TD on 2022/5/30.
//

#include "update.h"

namespace doodle::database_n {

class update_data::impl {
};
update_data::update_data(const std::vector<entt::entity> &in_data)
    : p_i(std::make_unique<impl>()) {
}
update_data::~update_data() = default;
void update_data::init() {
}
void update_data::succeeded() {
}
void update_data::failed() {
}
void update_data::aborted() {
}
void update_data::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
}
}  // namespace doodle::database_n
