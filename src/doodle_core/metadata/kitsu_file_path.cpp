#include "kitsu_file_path.h"

namespace doodle {
std::vector<kitsu_file_path> kitsu_file_path::select_all(
    const sql_connection_ptr& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
) {
  return {};
}
void kitsu_file_path::create_table(const sql_connection_ptr& in_comm) {}
std::vector<bool> kitsu_file_path::filter_exist(
    const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task
) {
  return {};
}
void kitsu_file_path::insert(
    const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task,
    const std::map<entt::entity, boost::uuids::uuid>& in_map_id
) {}
void kitsu_file_path::update(const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task) {}
void kitsu_file_path::delete_by_ids(const sql_connection_ptr& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {}
}  // namespace doodle