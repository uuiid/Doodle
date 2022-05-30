//
// Created by TD on 2022/5/30.
//

#include "select.h"
#include <thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <thread_pool/thread_pool.h>
namespace doodle {
namespace database_n {
class select::impl {
 public:
  FSys::path project;
  bool only_ctx;
  std::future<void> result;
};

select::select(const select::arg& in_arg) : p_i(std::make_unique<impl>()) {
  p_i->project  = in_arg.project_path;
  p_i->only_ctx = in_arg.only_ctx;
}
select::~select() = default;

void select::init() {
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("加载数据");
  k_msg.set_state(k_msg.run);
  p_i->result = g_thread_pool().enqueue([this]() {

    if (!p_i->only_ctx) {
      this->select_db();
    }
    this->select_ctx();
  });
}
void select::succeeded() {
}
void select::failed() {
}
void select::aborted() {
}
void select::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
}
void select::select_db() {
}
void select::select_ctx() {
}
bool select::chick_table() {
  return false;
}
void select::update() {
}
}  // namespace database_n
}  // namespace doodle
