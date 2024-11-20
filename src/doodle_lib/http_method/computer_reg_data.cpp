//
// Created by TD on 24-7-25.
//
#include "computer_reg_data.h"

namespace doodle::http {
computer_reg_data_manager& computer_reg_data_manager::get() {
  static computer_reg_data_manager l_manager;
  return l_manager;
}
void computer_reg_data_manager::reg(computer_reg_data_ptr in_data) {
  std::lock_guard l_lock(mutex_);
  computer_reg_datas_.emplace_back(in_data);
  // clear_old();
}
std::vector<computer_reg_data_ptr> computer_reg_data_manager::list() {
  std::lock_guard l_lock(mutex_);
  std::vector<computer_reg_data_ptr> l_out{};
  for (auto& it : computer_reg_datas_) {
    if (auto l_ptr = it.lock(); l_ptr) l_out.emplace_back(it);
  }
  return l_out;
}
void computer_reg_data_manager::clear_old() {
  for (auto it = computer_reg_datas_.begin(); it != computer_reg_datas_.end();) {
    if (it->expired()) {
      it = computer_reg_datas_.erase(it);
    } else {
      ++it;
    }
  }
}
void computer_reg_data_manager::clear(const computer_reg_data_ptr& in_data) {
  std::lock_guard l_lock(mutex_);
  for (auto it = computer_reg_datas_.begin(); it != computer_reg_datas_.end();) {
    if (it->lock() == in_data) {
      it = computer_reg_datas_.erase(it);
      return;
    }
    ++it;
  }
}
}  // namespace doodle::http