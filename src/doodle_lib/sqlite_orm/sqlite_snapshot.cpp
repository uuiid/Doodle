//
// Created by td_main on 2023/11/3.
//

#include "sqlite_snapshot.h"
namespace doodle::snapshot {

void sqlite_snapshot::operator()(std::underlying_type_t<entt::entity> in_underlying_type) {
  current_component_size_ = in_underlying_type;
  current_data_           = {};
}
void sqlite_snapshot::operator()(entt::entity in_entity) { current_entity_ = in_entity; }
void sqlite_snapshot::operator()(std::underlying_type_t<entt::entity>& in_underlying_type) {
  current_data_ = data_deque_.front();
  data_deque_.pop_front();
  current_component_size_ = current_data_->size();

  in_underlying_type      = current_component_size_;
}
void sqlite_snapshot::operator()(entt::entity& in_entity) {
  current_entity_ = current_data_->entity();
  in_entity       = current_entity_;
}

}  // namespace doodle::snapshot