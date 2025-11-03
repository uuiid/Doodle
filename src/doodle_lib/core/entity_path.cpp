#include "entity_path.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/doodle_macro.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <boost/core/noncopyable.hpp>

#include <cache.hpp>
#include <cache_policy.hpp>
#include <cctype>
#include <filesystem>
#include <lru_cache_policy.hpp>
#include <range/v3/view/filter.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

namespace doodle {

FSys::path get_entity_character_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Rig", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_character_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_character_rig_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}
/// 角色模型maya 路径
FSys::path get_entity_character_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_character_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_character_model_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}
namespace {
class cache_manger_user : public boost::noncopyable {
  template <typename Key, typename Value>
  using lru_cache_t = typename caches::fixed_sized_cache<Key, Value, caches::LRUCachePolicy>;

  struct cache_value {
    std::string user_abbreviation_;
    std::chrono::time_point<std::chrono::system_clock> time_point_;
  };

  using cache_type = lru_cache_t<uuid, cache_value>;
  cache_type cache_;

 public:
  cache_manger_user() : cache_(1024 * 1024) {}
  // copy

  void set(const uuid& id, const std::string& user_abbreviation) {
    cache_.Put(id, cache_value{user_abbreviation, std::chrono::system_clock::now()});
  }

  std::string get(const uuid& id) {
    if (auto l_value = cache_.TryGet(id);
        l_value.second && l_value.first->time_point_ + 300s > std::chrono::system_clock::now()) {
      return l_value.first->user_abbreviation_;
    } else {
      using namespace sqlite_orm;
      auto l_sql  = g_ctx().get<sqlite_database>();
      auto l_task = l_sql.impl_->storage_any_.select(
          &task::uuid_id_, from<task>(),
          where(c(&task::entity_id_) == id && c(&task::task_type_id_) == task_type::get_binding_id()), limit(1)
      );
      std::string l_user_abbreviation{};
      if (!l_task.empty()) {
        auto l_user = g_ctx().get<sqlite_database>().impl_->storage_any_.select(
            &person::last_name_, from<person>(),
            in(&person::uuid_id_,
               select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == l_task.front()))),
            limit(1)
        );

        if (!l_user.empty()) {
          l_user_abbreviation.reserve(8);
          l_user_abbreviation.push_back('_');
          for (auto&& l_ : l_user.front())
            if (std::isupper(l_)) l_user_abbreviation.push_back(l_);
        }
      }
      set(id, l_user_abbreviation);
      return l_user_abbreviation;
    }
  }
  void erase(const uuid& id) { cache_.Remove(id); }
};
cache_manger_user& g_get_cache_manger_user_abbreviation() {
  static cache_manger_user g_user_abbreviation{};
  return g_user_abbreviation;
}
}  // namespace

FSys::path get_entity_character_rig_maya_name(const entity_asset_extend& in_extend_) {
  auto l_entity_id = in_extend_.entity_id_;

  return fmt::format("Ch{}_rig{}.ma", in_extend_.bian_hao_, g_get_cache_manger_user_abbreviation().get(l_entity_id));
}
/// 道具模型maya 绑定路径
FSys::path get_entity_prop_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}/Rig", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
FSys::path get_entity_prop_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_prop_rig_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.pin_yin_ming_cheng_
  );
}
FSys::path get_entity_prop_rig_maya_name(const entity_asset_extend& in_extend_) {
  return fmt::format(
      "{}{}_rig{}.ma", in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_.empty() ? "" : "_" + in_extend_.ban_ben_,
      g_get_cache_manger_user_abbreviation().get(in_extend_.entity_id_)
  );
}

/// 道具模型maya 路径
FSys::path get_entity_prop_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
FSys::path get_entity_prop_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_prop_model_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.pin_yin_ming_cheng_
  );
}
/// 场景模型maya 绑定路径
FSys::path get_entity_ground_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_ground_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_rig_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}

/// 场景模型maya 路径
FSys::path get_entity_ground_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_ground_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_model_maya_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}
/// 角色模型ue 路径
FSys::path get_entity_character_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
FSys::path get_entity_character_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_character_ue_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_, in_extend_.pin_yin_ming_cheng_
  );
}
/// 角色模型 ue 名称
std::string get_entity_character_ue_name(const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_) {
  return fmt::format(
      "{}/Character/{}/Meshs/SK_Ch{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, bian_hao_
  );
}
std::string get_entity_character_ue_name(const entity_asset_extend& in_extend_) {
  return get_entity_character_ue_name(in_extend_.bian_hao_, in_extend_.pin_yin_ming_cheng_);
}
/// 道具模型ue 路径
FSys::path get_entity_prop_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_
) {
  return asset_root_path_ /
         fmt::format("Prop/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", gui_dang_, kai_shi_ji_shu_, gui_dang_, kai_shi_ji_shu_);
}
FSys::path get_entity_prop_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_prop_ue_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0)
  );
}
/// 道具模型 ue 名称
std::string get_entity_prop_ue_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/Prop/{}/Mesh/SK_{}{}{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, pin_yin_ming_cheng_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
std::string get_entity_prop_ue_name(const entity_asset_extend& in_extend_) {
  return get_entity_prop_ue_name(in_extend_.bian_hao_, in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_);
}
/// 场景模型ue 路径
FSys::path get_entity_ground_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("BG/JD{:02d}_{:02d}/BG{}/{}", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
FSys::path get_entity_ground_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_ue_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_, in_extend_.pin_yin_ming_cheng_
  );
}
/// 场景模型 ue map 名称
std::string get_entity_ground_ue_map_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/{}/Map/{}{}{}.umap", doodle_config::ue4_content, pin_yin_ming_cheng_, pin_yin_ming_cheng_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
std::string get_entity_ground_ue_map_name(const std::string& bian_hao_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_ue_map_name(bian_hao_, in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_);
}

///  场景模型 ue sk 名称
std::string get_entity_ground_ue_sk_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/{}/SK/SK_{}{}{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, pin_yin_ming_cheng_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
std::string get_entity_ground_ue_sk_name(const std::string& bian_hao_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_ue_sk_name(bian_hao_, in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_);
}
/// 场景名称 alembic 名称
std::string get_entity_ground_alembic_name(const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_) {
  return fmt::format("{}{}{}_Low.abc", pin_yin_ming_cheng_, ban_ben_.empty() ? "" : "_", ban_ben_);
}
std::string get_entity_ground_alembic_name(const entity_asset_extend& in_extend_) {
  return get_entity_ground_alembic_name(in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_);
}
std::string get_entity_ground_rig_name(const entity_asset_extend& in_extend_) {
  return fmt::format(
      "{}{}{}_Low.ma", in_extend_.pin_yin_ming_cheng_, in_extend_.ban_ben_.empty() ? "" : "_", in_extend_.ban_ben_
  );
}

/// 角色模型图片 路径
FSys::path get_entity_character_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_character_image_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_character_image_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}
/// 道具模型图片 路径
FSys::path get_entity_prop_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
FSys::path get_entity_prop_image_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_prop_image_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.pin_yin_ming_cheng_
  );
}
/// 场景模型图片 路径
FSys::path get_entity_ground_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_ground_image_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  return get_entity_ground_image_path(
      in_prj_.asset_root_path_, in_extend_.gui_dang_.value_or(0), in_extend_.kai_shi_ji_shu_.value_or(0),
      in_extend_.bian_hao_
  );
}
/// 获得解算资产路径
FSys::path get_entity_simulation_asset_path(const FSys::path& asset_root_path_) { return asset_root_path_ / "CFX"; }
FSys::path get_entity_simulation_asset_path(const project& in_prj_) {
  return get_entity_simulation_asset_path(in_prj_.asset_root_path_);
}
/// 动画镜头maya路径
FSys::path get_shots_animation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "ma";
  ;
}
FSys::path get_shots_animation_maya_path(const entity& episode_) {
  return get_shots_animation_maya_path(episode_.name_);
}
/// 动画镜头output路径
FSys::path get_shots_animation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "fbx" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
FSys::path get_shots_animation_output_path(const entity& episode_, const entity& shot_, const project& prj_) {
  return get_shots_animation_output_path(episode_.name_, shot_.name_, prj_.code_);
}
/// 解算镜头maya路径
FSys::path get_shots_simulation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "sim";
}
FSys::path get_shots_simulation_maya_path(const entity& episode_) {
  return get_shots_simulation_maya_path(episode_.name_);
}
/// 解算镜头output路径
FSys::path get_shots_simulation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "abc" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
FSys::path get_shots_simulation_output_path(const entity& episode_, const entity& shot_, const project& prj_) {
  return get_shots_simulation_output_path(episode_.name_, shot_.name_, prj_.code_);
}
/// 动画文件名称
std::string get_shots_animation_file_name(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
std::string get_shots_animation_file_name(const entity& episode_, const entity& shot_, const project& prj_) {
  return get_shots_animation_file_name(episode_.name_, shot_.name_, prj_.code_);
}
FSys::path conv_ue_game_path(const FSys::path& in_path) {
  auto l_str = in_path.generic_string();
  boost::replace_all(l_str, doodle_config::ue4_content, doodle_config::ue4_game);
  FSys::path l_path{l_str};
  l_path.replace_extension(l_path.stem());
  return l_path;
}

}  // namespace doodle