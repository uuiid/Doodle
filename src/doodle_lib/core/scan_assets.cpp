//
// Created by TD on 25-8-13.
//

#include "scan_assets.h"

#include "doodle_core/metadata/task_type.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/task.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
namespace doodle::scan_assets {
namespace {
working_file create_working_file(
    const FSys::path& in_path, const std::string& in_description, software_enum in_software_type
) {
  if (exists(in_path)) {
    auto l_uuid = FSys::software_flag_file(in_path);
    if (l_uuid.is_nil()) {
      l_uuid = core_set::get_set().get_uuid();
      FSys::software_flag_file(in_path, l_uuid);
    }
    return working_file{
        .uuid_id_       = l_uuid,
        .name_          = in_path.filename().generic_string(),
        .description_   = in_description,
        .size_          = boost::numeric_cast<std::int64_t>(FSys::file_size(in_path)),
        .path_          = in_path,
        .software_type_ = in_software_type,
    };
  }
  throw_exception(doodle_error{"{} 不存在", in_path});
}

std::shared_ptr<working_file> scan_character_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  FSys::path l_maya_path = in_prj.path_ / in_prj.asset_root_path_ / "Ch" /
                           fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                           fmt::format("Ch{}", in_extend.bian_hao_) / "Mod" /
                           fmt::format("Ch{}.ma", in_extend.bian_hao_);
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya文件", software_enum::maya));
  return nullptr;
}
std::shared_ptr<working_file> scan_character_unreal_engine(
    const project& in_prj, const entity_asset_extend& in_extend
) {
  FSys::path l_Ue_path = in_prj.path_ / in_prj.asset_root_path_ / "Ch" /
                         fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                         fmt::format("Ch{}", in_extend.bian_hao_) /
                         fmt::format("{}_UE5", in_extend.pin_yin_ming_cheng_) / doodle_config::ue4_content /
                         "Character" / in_extend.pin_yin_ming_cheng_ / "Meshs" /
                         fmt::format("SK_Ch{}.uasset", in_extend.bian_hao_);
  if (exists(l_Ue_path))
    return std::make_shared<working_file>(create_working_file(l_Ue_path, "UE4角色模型", software_enum::unreal_engine));
  return nullptr;
}

std::shared_ptr<working_file> scan_prop_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.path_ / in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) / in_extend.pin_yin_ming_cheng_ /
      fmt::format(
          "{}{}{}.ma", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(
        l_maya_path, "Maya道具模型", software_enum::maya
    ));
  return nullptr;
}
std::shared_ptr<working_file> scan_prop_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_UE_path =
      in_prj.path_ / in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("JD{:02d}_{:02d}_UE", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) / doodle_config::ue4_content /
      "Prop" / in_extend.pin_yin_ming_cheng_ / "Mesh" /
      fmt::format(
          "SK_{}{}{}.uasset", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_UE_path))
    return std::make_shared<working_file>(create_working_file(l_UE_path, "UE4道具模型", software_enum::unreal_engine));
  return nullptr;
}

std::shared_ptr<working_file> scan_scene_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.path_ / in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("BG{}", in_extend.bian_hao_) / "Mod" /
      fmt::format("BG{}{}{}_Low.ma", in_extend.bian_hao_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_);
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya场景文件", software_enum::maya));
  return nullptr;
}
std::shared_ptr<working_file> scan_scene_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_UE_path =
      in_prj.path_ / in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("BG{}", in_extend.bian_hao_) / in_extend.pin_yin_ming_cheng_ / doodle_config::ue4_content / "Map" /
      fmt::format(
          "{}{}{}.umap", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_UE_path))
    return std::make_shared<working_file>(create_working_file(l_UE_path, "UE4场景文件", software_enum::unreal_engine));
  return nullptr;
}
std::shared_ptr<working_file> scan_character_rig_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  FSys::path l_maya_path = in_prj.path_ / in_prj.asset_root_path_ / "Ch" /
                           fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                           fmt::format("Ch{}", in_extend.bian_hao_) / "Rig";
  std::string l_name = fmt::format("Ch{}_rig", in_extend.bian_hao_);

  if (exists(l_maya_path)) {
    for (auto&& i : FSys::directory_iterator(l_maya_path)) {
      if (i.is_regular_file() && i.path().extension() == ".ma" &&
          i.path().filename().generic_string().starts_with(l_name)) {
        return std::make_shared<working_file>(create_working_file(i.path(), "Maya角色绑定文件", software_enum::maya));
      }
    }
  }
  return nullptr;
}
std::shared_ptr<working_file> scan_prop_rig_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path = in_prj.path_ / in_prj.asset_root_path_ / "Prop" /
                     fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                     in_extend.pin_yin_ming_cheng_ / "Rig";
  auto l_name = fmt::format(
      "{}{}{}_rig", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
  );
  if (exists(l_maya_path)) {
    for (auto&& i : FSys::directory_iterator(l_maya_path)) {
      if (i.is_regular_file() && i.path().extension() == ".ma" &&
          i.path().filename().generic_string().starts_with(l_name)) {
        return std::make_shared<working_file>(create_working_file(i.path(), "Maya道具绑定文件", software_enum::maya));
      }
    }
  }
  return nullptr;
}
std::shared_ptr<working_file> scan_scene_rig_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.path_ / in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("BG{}", in_extend.bian_hao_) / "Mod" /
      fmt::format("BG{}{}{}_Low.ma", in_extend.bian_hao_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_);
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya地编绑定文件", software_enum::maya));
  return nullptr;
}
}  // namespace
std::shared_ptr<working_file> scan_maya(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_scene_id()) return scan_scene_maya(in_prj, in_extend);
  return nullptr;
}
std::shared_ptr<working_file> scan_unreal_engine(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_unreal_engine(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_unreal_engine(in_prj, in_extend);
  if (in_entity_type == asset_type::get_scene_id()) return scan_scene_unreal_engine(in_prj, in_extend);
  return nullptr;
}
std::shared_ptr<working_file> scan_rig_maya(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_rig_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_rig_maya(in_prj, in_extend);
  // if (in_entity_type == asset_type::get_scene_id()) return scan_scene_rig_maya(in_prj, in_extend);
  return nullptr;
}
std::shared_ptr<working_file> scan_sim_maya(const project& in_prj, const working_file& in_extend) {
  FSys::path l_maya_path = in_prj.path_ / in_prj.asset_root_path_ / "6-moxing" / "CFX" /
                           fmt::format("{}_cloth.ma", in_extend.path_.stem().generic_string());
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya布料模拟文件", software_enum::maya));
  return nullptr;
}
boost::asio::awaitable<void> scan_task(const task& in_task) {
  auto l_task_type_id = in_task.task_type_id_;
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_prj          = l_sql.get_by_uuid<project>(in_task.project_id_);
  auto l_entt         = l_sql.get_by_uuid<entity>(in_task.entity_id_);
  auto l_extend       = l_sql.get_entity_asset_extend(l_entt.uuid_id_).value();

  if (l_task_type_id == task_type::get_character_id() || l_task_type_id == task_type::get_ground_model_id()) {
    if (auto l_file = scan_maya(l_prj, l_entt.entity_type_id_, l_extend); l_file) {
      l_file->task_id_   = in_task.uuid_id_;
      l_file->entity_id_ = in_task.entity_id_;
      if (!l_sql.uuid_to_id<working_file>(l_file->uuid_id_)) co_await l_sql.install(l_file);
    }
    if (auto l_file = scan_unreal_engine(l_prj, l_entt.entity_type_id_, l_extend); l_file) {
      l_file->task_id_   = in_task.uuid_id_;
      l_file->entity_id_ = in_task.entity_id_;
      if (!l_sql.uuid_to_id<working_file>(l_file->uuid_id_)) co_await l_sql.install(l_file);
    }
  } else if (l_task_type_id == task_type::get_binding_id()) {
    if (auto l_file = scan_rig_maya(l_prj, l_entt.entity_type_id_, l_extend); l_file) {
      l_file->task_id_   = in_task.uuid_id_;
      l_file->entity_id_ = in_task.entity_id_;
      if (!l_sql.uuid_to_id<working_file>(l_file->uuid_id_)) co_await l_sql.install(l_file);
    }
  } else if (l_task_type_id == task_type::get_simulation_id()) {
    using namespace sqlite_orm;
    auto l_tasks = l_sql.impl_->storage_any_.get_all<task>(
        where(c(&task::entity_id_) == in_task.entity_id_ && c(&task::task_type_id_) == task_type::get_binding_id())
    );
    if (l_tasks.empty()) co_return;  // 没有绑定任务, 无法进行模拟

    auto l_work_file = l_sql.get_working_file_by_task(l_tasks.front().uuid_id_);
    if (auto l_file = scan_sim_maya(l_prj, l_work_file.front()); l_file) {
      l_file->task_id_   = in_task.uuid_id_;
      l_file->entity_id_ = in_task.entity_id_;
      if (!l_sql.uuid_to_id<working_file>(l_file->uuid_id_)) co_await l_sql.install(l_file);
    }
  }
}

}  // namespace doodle::scan_assets