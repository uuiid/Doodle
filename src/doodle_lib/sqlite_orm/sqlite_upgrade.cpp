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

#include "core/core_set.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>

#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 11;
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
struct entity_asset_extend_old {
  DOODLE_BASE_FIELDS();
  uuid entity_id_;

  std::optional<std::int32_t> ji_shu_lie_;
  std::string deng_ji_;
  std::optional<std::int32_t> gui_dang_;
  std::string bian_hao_;
  std::string pin_yin_ming_cheng_;
  std::string ban_ben_;
  std::optional<std::int32_t> ji_du_;
  std::optional<std::int32_t> kai_shi_ji_shu_;
  std::optional<std::int32_t> chang_ci_{};
};
struct project_data {
  std::vector<std::pair<entity, entity_asset_extend_old>> entity_asset_extends_;
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

    if (in_data.pragma().user_version() == 10) {
      in_data.drop_table("entity_asset_extend_2");
      in_data.sync_schema();

      using namespace orm;
      in_data.reg_table<entity_asset_extend_old>("entity_asset_extend")
          .add_column("id", &entity_asset_extend_old::id_, primary_key(), autoincrement())
          .add_column("uuid", &entity_asset_extend_old::uuid_id_, unique(), not_null())
          .add_column("entity_id", &entity_asset_extend_old::entity_id_, not_null())
          .add_column("ji_shu_lie", &entity_asset_extend_old::ji_shu_lie_)
          .add_column("deng_ji", &entity_asset_extend_old::deng_ji_)
          .add_column("gui_dang", &entity_asset_extend_old::gui_dang_)
          .add_column("bian_hao", &entity_asset_extend_old::bian_hao_)
          .add_column("pin_yin_ming_cheng", &entity_asset_extend_old::pin_yin_ming_cheng_)
          .add_column("ban_ben", &entity_asset_extend_old::ban_ben_)
          .add_column("ji_du", &entity_asset_extend_old::ji_du_)
          .add_column("kai_shi_ji_shu", &entity_asset_extend_old::kai_shi_ji_shu_)
          .add_column("chang_ci", &entity_asset_extend_old::chang_ci_)
          .add_foreign_key(&entity_asset_extend_old::entity_id_, &entity::uuid_id_, foreign_key_action::cascade);

      auto l_entitys =
          select(in_data)
              .columns(object<entity>(), object<entity_asset_extend_old>())
              .from<entity>()
              .where(c(&entity::entity_type_id_)
                         .in(
                             {asset_type::get_sequence_id(), asset_type::get_character_id(), asset_type::get_prop_id(),
                              asset_type::get_effect_id(), asset_type::get_ground_id(),
                              asset_type::get_scene_asset_id(), asset_type::get_ai_id()}
                         ))
              .left_outer_join<entity_asset_extend_old>(&entity_asset_extend_old::entity_id_, &entity::uuid_id_)()
              .to_vector();
      std::map<uuid, project_data> l_project_datas{};
      for (auto&& [l_entity, l_ext] : l_entitys) {
        if (l_entity.canceled_) continue;
        if (l_entity.entity_type_id_ == asset_type::get_sequence_id()) {
          l_project_datas[l_entity.project_id_].eps_entities_.emplace(l_entity.name_, l_entity);
        } else if (l_entity.entity_type_id_ == asset_type::get_character_id() ||
                   l_entity.entity_type_id_ == asset_type::get_prop_id() ||
                   l_entity.entity_type_id_ == asset_type::get_effect_id() ||
                   l_entity.entity_type_id_ == asset_type::get_ground_id() ||
                   l_entity.entity_type_id_ == asset_type::get_ai_id() ||
                   l_entity.entity_type_id_ == asset_type::get_scene_asset_id()) {
          l_project_datas[l_entity.project_id_].assets_entities_.emplace_back(l_entity);
          if (l_ext)
            l_project_datas[l_entity.project_id_].entity_asset_extends_.emplace_back(std::make_pair(l_entity, l_ext));
        }
      }
      auto l_g                              = in_data.transaction();
      std::shared_ptr<entity> l_entitys_ptr = std::make_shared<entity>();
      for (auto&& [l_project_id, l_data] : l_project_datas) {
        for (auto&& [l_entity, l_entity_ext_] : l_data.entity_asset_extends_) {
          auto l_eps_name =
              l_entity_ext_.ji_shu_lie_ ? fmt::format("EP{:03d}", *l_entity_ext_.ji_shu_lie_) : std::string{};
          auto l_kai_shi_ji_shu =
              l_entity_ext_.kai_shi_ji_shu_ ? fmt::format("EP{:03d}", *l_entity_ext_.kai_shi_ji_shu_) : std::string{};
          if (!l_eps_name.empty() && !l_data.eps_entities_.contains(l_eps_name)) {
            *l_entitys_ptr = entity{
                .uuid_id_        = core_set::get_set().get_uuid(),
                .name_           = l_eps_name,
                .project_id_     = l_project_id,
                .entity_type_id_ = asset_type::get_sequence_id(),
                .created_by_     = l_entity.created_by_,
            };
            in_data.install_unsafe<entity>(l_entitys_ptr);
            l_data.eps_entities_.emplace(l_eps_name, *l_entitys_ptr);
          }
          if (!l_kai_shi_ji_shu.empty() && !l_data.eps_entities_.contains(l_kai_shi_ji_shu)) {
            *l_entitys_ptr = entity{
                .uuid_id_        = core_set::get_set().get_uuid(),
                .name_           = l_kai_shi_ji_shu,
                .project_id_     = l_project_id,
                .entity_type_id_ = asset_type::get_sequence_id(),
                .created_by_     = l_entity.created_by_,
            };
            in_data.install_unsafe<entity>(l_entitys_ptr);
            l_data.eps_entities_.emplace(l_kai_shi_ji_shu, *l_entitys_ptr);
          }

          auto l_entity_ext_new = entity_asset_extend{
              .uuid_id_            = l_entity_ext_.uuid_id_,
              .entity_id_          = l_entity.uuid_id_,
              .ji_shu_lie_         = l_eps_name.empty() ? uuid{} : l_data.eps_entities_.at(l_eps_name).uuid_id_,
              .deng_ji_            = l_entity_ext_.deng_ji_,
              .gui_dang_           = l_entity_ext_.gui_dang_,
              .bian_hao_           = l_entity_ext_.bian_hao_,
              .pin_yin_ming_cheng_ = l_entity_ext_.pin_yin_ming_cheng_,
              .ban_ben_            = l_entity_ext_.ban_ben_,
              .ji_du_              = l_entity_ext_.ji_du_,
              .kai_shi_ji_shu_ = l_kai_shi_ji_shu.empty() ? uuid{} : l_data.eps_entities_.at(l_kai_shi_ji_shu).uuid_id_,
              .chang_ci_       = l_entity_ext_.chang_ci_,
          };
          auto l_entity_ext_ptr = std::make_shared<entity_asset_extend>(l_entity_ext_new);
          in_data.install_unsafe<entity_asset_extend>(l_entity_ext_ptr);
        }
      }

      l_g.commit();
      in_data.drop_table("entity_asset_extend");
      in_data.vacuum();
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