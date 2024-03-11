//
// Created by td_main on 2023/11/3.
//

#include "sqlite_snapshot.h"
namespace doodle::snapshot {

void sqlite_snapshot::operator()(std::underlying_type_t<entt::entity> in_underlying_type) {
}
void sqlite_snapshot::operator()(entt::entity in_entity) {}
void sqlite_snapshot::operator()(std::underlying_type_t<entt::entity>& in_underlying_type) {

}
void sqlite_snapshot::operator()(entt::entity& in_entity) {}

}  // namespace doodle::snapshot