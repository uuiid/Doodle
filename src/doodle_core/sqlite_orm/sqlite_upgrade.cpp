//
// Created by TD on 25-5-15.
//

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_upgrade.h>

#include <boost/hana/ext/std/tuple.hpp>

#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>
#include <type_traits>
namespace doodle::details {

struct upgrade_init_t : sqlite_upgrade {
  // 755c9edd-9481-4145-ab43-21491bdf2739
  static constexpr uuid g_open_id{
      {0x75, 0x5c, 0x9e, 0xdd, 0x94, 0x81, 0x41, 0x45, 0xab, 0x43, 0x21, 0x49, 0x1b, 0xdf, 0x27, 0x39}
  };
  // 0196eb9d5dc0727d8a751b05dea8494d
  static constexpr uuid g_lable_id{
      {0x01, 0x96, 0xeb, 0x9d, 0x5d, 0xc0, 0x72, 0x7d, 0x8a, 0x75, 0x1b, 0x05, 0xde, 0xa8, 0x49, 0x4d}
  };
  // 5159f210-7ec8-40e3-b8c9-2a06d0b4b116
  static constexpr uuid g_closed_id{
      {0x51, 0x59, 0xf2, 0x10, 0x7e, 0xc8, 0x40, 0xe3, 0xb8, 0xc9, 0x2a, 0x06, 0xd0, 0xb4, 0xb1, 0x16}
  };
  explicit upgrade_init_t(const FSys::path& in_path) {}
  void upgrade(const std::shared_ptr<sqlite_database_impl>& in_data) override {
    if (in_data->uuid_to_id<assets_helper::database_t>(g_lable_id) == 0) {
      auto l_label      = std::make_shared<assets_helper::database_t>();
      l_label->uuid_id_ = g_lable_id;
      l_label->label_   = "标签";
      in_data->install_unsafe<assets_helper::database_t>(l_label);
      in_data->storage_any_.pragma.user_version(1);  // 初次初始化, 设置版本号为1
    }
    if (in_data->uuid_to_id<project_status>(g_open_id) == 0) {
      auto l_s      = std::make_shared<project_status>();
      l_s->uuid_id_ = g_open_id;
      l_s->name_    = "Open";
      l_s->color_   = "#000000";
      in_data->install_unsafe<project_status>(l_s);
    }
    if (in_data->uuid_to_id<project_status>(g_closed_id) == 0) {
      auto l_s      = std::make_shared<project_status>();
      l_s->uuid_id_ = g_closed_id;
      l_s->name_    = "Closed";
      l_s->color_   = "#000000";
      in_data->install_unsafe<project_status>(l_s);
    }
#define DOODLE_ASSET_TYPE(name, sql_name)                                    \
  if (in_data->uuid_to_id<asset_type>(asset_type::get_##name##_id()) == 0) { \
    auto l_s       = std::make_shared<asset_type>();                         \
    l_s->uuid_id_  = asset_type::get_##name##_id();                          \
    l_s->name_     = #sql_name;                                              \
    l_s->archived_ = true;                                                   \
    in_data->install_unsafe<asset_type>(l_s);                                \
  }
    DOODLE_ASSET_TYPE(scene, Scene)
    DOODLE_ASSET_TYPE(sequence, Sequence)
    DOODLE_ASSET_TYPE(shot, Shot)
    DOODLE_ASSET_TYPE(edit, Edit)
    DOODLE_ASSET_TYPE(concept, Concept)
    DOODLE_ASSET_TYPE(episode, Episode)
#undef DOODLE_ASSET_TYPE
#define DOODLE_TASK_TYPE(name, sql_name)                                   \
  if (in_data->uuid_to_id<task_type>(task_type::get_##name##_id()) == 0) { \
    auto l_s       = std::make_shared<task_type>();                        \
    l_s->uuid_id_  = task_type::get_##name##_id();                         \
    l_s->name_     = #sql_name;                                            \
    l_s->archived_ = true;                                                 \
    l_s->color_    = "#999999";                                            \
    in_data->install_unsafe<task_type>(l_s);                               \
  }
    DOODLE_TASK_TYPE(original_painting, 原画)
    DOODLE_TASK_TYPE(character, 角色)
    DOODLE_TASK_TYPE(ground_model, 地编模型)
    DOODLE_TASK_TYPE(binding, 绑定)
    DOODLE_TASK_TYPE(simulation, 解算资产)
    DOODLE_TASK_TYPE(effect, 特效资产)
#undef DOODLE_TASK_TYPE
    using namespace sqlite_orm;
    in_data->storage_any_.remove_all<working_file>(where(not_in(
        &working_file::uuid_id_,
        union_(select(&working_file_task_link::working_file_id_), select(&working_file_entity_link::working_file_id_))
    )));
  }
};  // namespace doodle::details

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t(const FSys::path& in_path) {}
  void upgrade(const std::shared_ptr<sqlite_database_impl>& in_data) override {}
  ~upgrade_2_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_init_t>(in_db_path);
}
std::shared_ptr<sqlite_upgrade> upgrade_1(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_2_t>(in_db_path);
}

}  // namespace doodle::details