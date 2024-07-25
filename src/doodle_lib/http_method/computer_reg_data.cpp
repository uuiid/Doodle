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
  std::lock_guard<std::mutex> l_lock(mutex_);
  computer_reg_datas_.insert(in_data);
  clear_old();
}
std::set<computer_reg_data_ptr> computer_reg_data_manager::list() {
  std::lock_guard<std::mutex> l_lock(mutex_);
  return computer_reg_datas_;
}
void computer_reg_data_manager::clear_old() {
  for (auto it = computer_reg_datas_.begin(); it != computer_reg_datas_.end();) {
    if ((*it)->websocket_data_.expired()) {
      it = computer_reg_datas_.erase(it);
    } else {
      ++it;
    }
  }
}

}