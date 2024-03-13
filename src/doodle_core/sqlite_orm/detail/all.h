//
// Created by TD on 2024/3/13.
//

#pragma once
namespace doodle::snapshot {
/**
 *
 * void create_table(const conn_ptr& in_conn) {}
 * std::shared_ptr<void> begin_save(const conn_ptr& in_conn) {}
 * void save(const database& in_com, entt::entity& in_entity, std::shared_ptr<void>& in_pre, const conn_ptr& in_conn) {}
 * void destroy(const std::vector<std::int64_t>& in_vector, const conn_ptr& in_conn) {}
 *
 * std::underlying_type_t<entt::entity> get_size(const conn_ptr& in_conn) {}
 * using pre_rus_t =
 * std::shared_ptr<void> begin_load(const conn_ptr& in_conn) {}
 * void load_entt(entt::entity& in_entity, std::shared_ptr<void>& in_pre) {}
 * void load_com(database& in_entity, std::shared_ptr<void>& in_pre) {}
 * bool has_table(const conn_ptr& in_conn) {}
 */
void reg_database();
void reg_server_task_info();

}  // namespace doodle::snapshot