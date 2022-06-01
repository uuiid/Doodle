//
// Created by TD on 2022/5/30.
//

#include "update.h"

namespace doodle::database_n {
namespace {
/**
 * @brief 组件数据
 */
class com_data {
 public:
  com_data(entt::entity in_entt,
           std::uint32_t in_id,
           std::string in_str)
      : entt_(in_entt),
        com_id(in_id),
        json_data(std::move(in_str)) {}

  entt::entity entt_{};
  std::uint32_t com_id{};
  std::string json_data{};
};
}  // namespace
class update_data::impl {
 public:
  std::vector<entt::entity> entt_list{};
};
update_data::update_data(const std::vector<entt::entity> &in_data)
    : p_i(std::make_unique<impl>()) {
  p_i->entt_list = in_data;
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
