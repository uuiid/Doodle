//
// Created by TD on 2022/5/30.
//

#include "observe.h"

namespace doodle {
namespace database_n {
class observe::impl {
 public:
};
observe::observe(const std::vector<entt::entity> &in_data) {
}
observe::~observe() {
}
void observe::init() {
}
void observe::succeeded() {
}
void observe::failed() {
}
void observe::aborted() {
}
void observe::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
}
}  // namespace database_n
}  // namespace doodle
