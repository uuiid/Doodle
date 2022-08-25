//
// Created by TD on 2021/12/25.
//

#include "ue4_exe.h"
#include <doodle_core/thread_pool/process_message.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/filesystem_extend.h>

namespace doodle {
namespace ue4_comm {

sequencer_comm::sequencer_comm(
    const entt::handle &in_handle,
    std::vector<entt::handle> &in_vector
) {
}
void sequencer_comm::init() {
}
void sequencer_comm::succeeded() {
}
void sequencer_comm::failed() {
}
void sequencer_comm::aborted() {
}
void sequencer_comm::update(
    const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &,
    void *data
) {
}
import_file_comm::import_file_comm(const entt::handle &in_handle, std::vector<entt::handle> &in_vector) {
}
void import_file_comm::init() {
}
void import_file_comm::succeeded() {
}
void import_file_comm::failed() {
}
void import_file_comm::aborted() {
}
void import_file_comm::update(
    const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &,
    void *data
) {
}
}  // namespace ue4_comm
ue4_exe::ue4_exe(const entt::handle &in_handle) {
}
void ue4_exe::init() {
}
void ue4_exe::succeeded() {
}
void ue4_exe::failed() {
}
void ue4_exe::aborted() {
}
void ue4_exe::update(const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &, void *data) {
}
}  // namespace doodle
