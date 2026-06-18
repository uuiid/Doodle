//
// Created by TD on 25-5-15.
//
//

#include "doodle_core/metadata/entity.h"
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include <boost/hana/ext/std/tuple.hpp>

#include "sqlite_orm/orm/bind_value.h"
#include "sqlite_orm/orm/fwd.h"
#include "sqlite_orm/orm/select.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <tuple>
#include <vector>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 8;
}

struct upgrade_init_t : sqlite_upgrade {
  explicit upgrade_init_t(const FSys::path& in_path) {}

  static void full_fts_sync(sqlite_database& in_data) {
    using namespace orm;
    insert(in_data).into<entity_fts>().set(c(any_column<entity_fts>()) = "rebuild")();
  }

  void upgrade(sqlite_database& in_data) override {
    if (in_data.pragma().user_version() != 0) return;
    in_data.sync_schema();
    in_data.pragma().user_version(g_current_version);

#define DOODLE_ASSET_TYPE(class_name)                      \
  for (const auto& v : class_name::get_all_constant()) {   \
    if (in_data.uuid_to_id<class_name>(v.uuid_id_) == 0) { \
      auto l_s = std::make_shared<class_name>(v);          \
      in_data.install_unsafe<class_name>(l_s);             \
    }                                                      \
  }

    DOODLE_ASSET_TYPE(project_status)
    DOODLE_ASSET_TYPE(assets_helper::database_t)
    DOODLE_ASSET_TYPE(asset_type)
    DOODLE_ASSET_TYPE(task_type)

#undef DOODLE_ASSET_TYPE
  }
};  // namespace doodle::details

namespace {
struct project_data {
  std::vector<entity_asset_extend> entity_asset_extends_;
  std::map<std::string, entity> eps_entities_;
  std::vector<entity> assets_entities_;
};
}  // namespace

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t(const FSys::path& in_path) {}
  void upgrade(sqlite_database& in_data) override {
    if (in_data.pragma().user_version() == 2) {
      upgrade_init_t::full_fts_sync(in_data);
      in_data.pragma().user_version(g_current_version);
    }
    if (in_data.pragma().user_version() == 7) {
      using namespace orm;
      auto l_entitys = select(in_data)
                           .columns(object<entity>(), object<entity_asset_extend>())
                           .from<entity>()
                           .where(c(&entity::entity_type_id_)
                                      .in(
                                          {asset_type::get_shot_id(), asset_type::get_character_id(),
                                           asset_type::get_prop_id(), asset_type::get_effect_id(),
                                           asset_type::get_ground_id(), asset_type::get_scene_asset_id()}
                                      ))
                           .left_outer_join<entity_asset_extend>(&entity_asset_extend::entity_id_, &entity::uuid_id_)()
                           .to_vector();
      std::map<uuid, project_data> l_project_datas{};
      for (auto&& [l_entity, l_ext] : l_entitys) {
        if (l_entity.canceled_) continue;
        if (l_entity.entity_type_id_ == asset_type::get_shot_id()) {
          l_project_datas[l_entity.project_id_].eps_entities_.emplace(l_entity.name_, l_entity);
        } else if (l_entity.entity_type_id_ == asset_type::get_character_id() ||
                   l_entity.entity_type_id_ == asset_type::get_prop_id() ||
                   l_entity.entity_type_id_ == asset_type::get_effect_id() ||
                   l_entity.entity_type_id_ == asset_type::get_ground_id() ||
                   l_entity.entity_type_id_ == asset_type::get_scene_asset_id()) {
          l_project_datas[l_entity.project_id_].assets_entities_.emplace_back(l_entity);
          if (l_ext) l_project_datas[l_entity.project_id_].entity_asset_extends_.emplace_back(l_ext);
        }
      }
    }

    in_data.pragma().user_version(g_current_version);
  }
  ~upgrade_2_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_init_t>(in_db_path);
}
std::shared_ptr<sqlite_upgrade> upgrade_1(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_2_t>(in_db_path);
}

}  // namespace doodle::details