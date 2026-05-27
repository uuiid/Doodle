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

#include "sqlite_orm/orm/select.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 5;
}

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

  static void full_fts_sync(sqlite_database& in_data) {
    using namespace orm;
    insert(in_data).into<entity_fts>().set(c(any_column<entity_fts>()) = "rebuild")();
  }

  void upgrade(sqlite_database& in_data) override {
    if (in_data.pragma().user_version() != 0) return;
    in_data.sync_schema();
    in_data.pragma().user_version(g_current_version);
    if (in_data.uuid_to_id<assets_helper::database_t>(g_lable_id) == 0) {
      auto l_label      = std::make_shared<assets_helper::database_t>();
      l_label->uuid_id_ = g_lable_id;
      l_label->label_   = "标签";
      in_data.install_unsafe<assets_helper::database_t>(l_label);
    }
    if (in_data.uuid_to_id<project_status>(g_open_id) == 0) {
      auto l_s      = std::make_shared<project_status>();
      l_s->uuid_id_ = g_open_id;
      l_s->name_    = "Open";
      l_s->color_   = "#000000";
      in_data.install_unsafe<project_status>(l_s);
    }
    if (in_data.uuid_to_id<project_status>(g_closed_id) == 0) {
      auto l_s      = std::make_shared<project_status>();
      l_s->uuid_id_ = g_closed_id;
      l_s->name_    = "Closed";
      l_s->color_   = "#000000";
      in_data.install_unsafe<project_status>(l_s);
    }
#define DOODLE_ASSET_TYPE(name, sql_name)                                   \
  if (in_data.uuid_to_id<asset_type>(asset_type::get_##name##_id()) == 0) { \
    auto l_s       = std::make_shared<asset_type>();                        \
    l_s->uuid_id_  = asset_type::get_##name##_id();                         \
    l_s->name_     = #sql_name;                                             \
    l_s->archived_ = true;                                                  \
    in_data.install_unsafe<asset_type>(l_s);                                \
  }
    DOODLE_ASSET_TYPE(scene, Scene)
    DOODLE_ASSET_TYPE(sequence, Sequence)
    DOODLE_ASSET_TYPE(shot, Shot)
    DOODLE_ASSET_TYPE(edit, Edit)
    DOODLE_ASSET_TYPE(concept, Concept)
    DOODLE_ASSET_TYPE(episode, Episode)
#undef DOODLE_ASSET_TYPE
    // #define DOODLE_TASK_TYPE(name, sql_name)                                  \
//   if (in_data.uuid_to_id<task_type>(task_type::get_##name##_id()) == 0) { \
//     auto l_s       = std::make_shared<task_type>();                       \
//     l_s->uuid_id_  = task_type::get_##name##_id();                        \
//     l_s->name_     = #sql_name;                                           \
//     l_s->archived_ = true;                                                \
//     l_s->color_    = "#999999";                                           \
//     in_data.install_unsafe<task_type>(l_s);                               \
//   }
    //     DOODLE_TASK_TYPE(original_painting, 原画)
    //     DOODLE_TASK_TYPE(character, 角色)
    //     DOODLE_TASK_TYPE(ground_model, 地编模型)
    //     DOODLE_TASK_TYPE(binding, 绑定)
    //     DOODLE_TASK_TYPE(simulation, 解算资产)
    //     DOODLE_TASK_TYPE(effect_asset, 特效资产)
    // #undef DOODLE_TASK_TYPE
  }
};  // namespace doodle::details

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t(const FSys::path& in_path) {}
  void upgrade(sqlite_database& in_data) override {
    // if (in_data->storage_any_.pragma.user_version() == 1) {
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_insert_trigger");
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_update_trigger");
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_delete_trigger");
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_insert_trigger2");
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_update_trigger2");
    //   in_data->storage_any_.drop_trigger_if_exists("entity_fts_delete_trigger2");
    //   in_data->storage_any_.drop_table_if_exists("entity_fts");
    //   in_data->storage_any_.drop_view_if_exists("entity_asset_view");

    //   in_data->sync_schema();
    //   upgrade_init_t::full_fts_sync(in_data);
    //   in_data->storage_any_.pragma.user_version(g_current_version);
    // }

    if (in_data.pragma().user_version() == 2) {
      upgrade_init_t::full_fts_sync(in_data);
      in_data.pragma().user_version(g_current_version);
    }

    if (in_data.pragma().user_version() == 4) {
      auto l_g = in_data.transaction();
      in_data.exec(R"(alter table seedance2_task
    add completion_tokens integer default 200000 not null;)");
      in_data.exec(R"(alter table person
    add max_completion_tokens integer;)");
      std::vector<std::tuple<uuid, std::int64_t>> l_task_tokens;
      {
        using namespace orm;
        for (auto&& [l_uuid, l_res] : orm::select_t(in_data)
                                          .columns(&seedance2::task::uuid_id_, &seedance2::task::data_response_)
                                          .from<seedance2::task>()()) {
          l_task_tokens.emplace_back(l_uuid, 0);
          if (l_res.is_null()) continue;
          if (!l_res.contains("usage") || !l_res.at("usage").contains("completion_tokens")) continue;
          l_task_tokens.back() = std::make_tuple(l_uuid, l_res.at("usage").at("completion_tokens").get<std::int64_t>());
        }
        for (auto&& [l_uuid, l_tokens] : l_task_tokens)
          orm::update(in_data)
              .from<seedance2::task>()
              .set(c(&seedance2::task::completion_tokens_) = l_tokens)
              .where(c(&seedance2::task::uuid_id_) = l_uuid)();
      }
      l_g.commit();
      in_data.sync_schema();
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