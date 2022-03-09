//
// Created by TD on 2021/12/25.
//

#include "ue4_exe.h"
#include <thread_pool/process_message.h>
#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/core/core_set.h>

#include <doodle_lib/thread_pool/thread_pool.h>
namespace doodle {
namespace ue4_comm {

sequencer_comm::sequencer_comm(
    const entt::handle &in_handle,
    std::vector<entt::handle> &in_vector) {
}
void sequencer_comm::init() {
}
void sequencer_comm::succeeded() {
}
void sequencer_comm::failed() {
}
void sequencer_comm::aborted() {
}
void sequencer_comm::update(const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &, void *data) {
}
}  // namespace ue4_comm
}  // namespace doodle
