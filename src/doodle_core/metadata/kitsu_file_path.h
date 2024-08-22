#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/uuid/uuid.hpp>
namespace doodle {

class kitsu_file_path {
 public:
  kitsu_file_path()  = default;
  ~kitsu_file_path() = default;

  boost::uuids::uuid id_;
  FSys::path path_{};

 public:
  static std::vector<kitsu_file_path> select_all(
      const sql_connection_ptr& in_comm, const std::map<boost::uuids::uuid, entt::entity>& in_map_id
  );
  static void create_table(const sql_connection_ptr& in_comm);

  // 过滤已经存在的任务
  static std::vector<bool> filter_exist(const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task);
  static void insert(
      const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task,
      const std::map<entt::entity, boost::uuids::uuid>& in_map_id
  );
  static void update(const sql_connection_ptr& in_comm, const std::vector<kitsu_file_path>& in_task);
  static void delete_by_ids(const sql_connection_ptr& in_comm, const std::vector<boost::uuids::uuid>& in_ids);
};
}  // namespace doodle