//
// Created by TD on 25-8-13.
//

#include "scan_assets.h"

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/working_file.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/task.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>

#include <boost/asio/awaitable.hpp>

#include "core/scan_assets.h"
#include <filesystem>
#include <memory>

namespace doodle::scan_assets {
namespace {

FSys::path scan_character_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path = in_prj.asset_root_path_ / "Ch" /
                fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
                fmt::format("Ch{}", in_extend.bian_hao_) / "Mod" / fmt::format("Ch{}.ma", in_extend.bian_hao_);
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_character_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path = in_prj.asset_root_path_ / "Ch" /
                fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
                fmt::format("Ch{}", in_extend.bian_hao_) / fmt::format("{}_UE5", in_extend.pin_yin_ming_cheng_) /
                doodle_config::ue4_content / "Character" / in_extend.pin_yin_ming_cheng_ / "Meshs" /
                fmt::format("SK_Ch{}.uasset", in_extend.bian_hao_);
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}

FSys::path scan_prop_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      in_extend.pin_yin_ming_cheng_ /
      fmt::format(
          "{}{}{}.ma", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_prop_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("JD{:02d}_{:02d}_UE", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      doodle_config::ue4_content / "Prop" / in_extend.pin_yin_ming_cheng_ / "Mesh" /
      fmt::format(
          "SK_{}{}{}.uasset", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}

FSys::path scan_scene_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("BG{}", in_extend.bian_hao_) / "Mod" /
      fmt::format(
          "{}{}{}_Low.ma", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_scene_alembic(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("BG{}", in_extend.bian_hao_) / "Mod" /
      fmt::format(
          "{}{}{}_Low.abc", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_scene_unreal_engine_sk(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("BG{}", in_extend.bian_hao_) / in_extend.pin_yin_ming_cheng_ / doodle_config::ue4_content /
      in_extend.pin_yin_ming_cheng_ / "SK" /
      fmt::format(
          "SK_{}{}{}.uasset", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_scene_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_path =
      in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("BG{}", in_extend.bian_hao_) / in_extend.pin_yin_ming_cheng_ / doodle_config::ue4_content /
      in_extend.pin_yin_ming_cheng_ / "Map" /
      fmt::format(
          "{}{}{}.umap", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(in_prj.path_ / l_path)) return l_path;
  return {};
}
FSys::path scan_character_rig_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  FSys::path l_maya_path =
      in_prj.asset_root_path_ / "Ch" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      fmt::format("Ch{}", in_extend.bian_hao_) / "Rig";
  std::string l_name = fmt::format("Ch{}_rig", in_extend.bian_hao_);
  auto l_p           = in_prj.path_ / l_maya_path;
  if (exists(l_p)) {
    for (auto&& i : FSys::directory_iterator(l_p)) {
      if (i.is_regular_file() && i.path().extension() == ".ma" &&
          i.path().filename().generic_string().starts_with(l_name)) {
        return l_maya_path / i.path().filename();
      }
    }
  }
  return {};
}
FSys::path scan_prop_rig_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_.value_or(0), in_extend.kai_shi_ji_shu_.value_or(0)) /
      in_extend.pin_yin_ming_cheng_ / "Rig";
  auto l_name = fmt::format(
      "{}{}{}_rig", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
  );
  auto l_p = in_prj.path_ / l_maya_path;
  if (exists(l_p)) {
    for (auto&& i : FSys::directory_iterator(l_p)) {
      if (i.is_regular_file() && i.path().extension() == ".ma" &&
          i.path().filename().generic_string().starts_with(l_name)) {
        return l_maya_path / i.path().filename();
      }
    }
  }
  return {};
}

}  // namespace
FSys::path scan_maya(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_ground_id()) return scan_scene_maya(in_prj, in_extend);
  return {};
}
FSys::path scan_alembic(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend) {
  if (in_entity_type == asset_type::get_ground_id()) {
    return scan_scene_alembic(in_prj, in_extend);
  }
  return {};
}
FSys::path scan_unreal_engine(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_unreal_engine(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_unreal_engine(in_prj, in_extend);
  if (in_entity_type == asset_type::get_ground_id()) return scan_scene_unreal_engine(in_prj, in_extend);
  return {};
}
FSys::path scan_rig_maya(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend) {
  if (in_entity_type == asset_type::get_character_id()) return scan_character_rig_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_rig_maya(in_prj, in_extend);
  if (in_entity_type == asset_type::get_ground_id()) return scan_scene_maya(in_prj, in_extend);
  return {};
}
FSys::path scan_sim_maya(const project& in_prj, const working_file& in_extend) {
  FSys::path l_maya_path =
      in_prj.asset_root_path_ / "CFX" / fmt::format("{}_cloth.ma", in_extend.path_.stem().generic_string());
  if (exists(in_prj.path_ / l_maya_path)) return l_maya_path;
  return {};
}

void scan_add_linked_data(
    working_file& in_working_file, const std::shared_ptr<scan_result>& in_result, const project& in_prj,
    const entity& in_entt, const task& in_task
) {
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_p         = in_prj.path_ / in_working_file.path_;
  auto l_file_uuid = FSys::software_flag_file(l_p);
  if (l_sql.uuid_to_id<working_file>(l_file_uuid) == 0) {
    if (in_working_file.uuid_id_.is_nil()) {
      in_working_file.uuid_id_ = core_set::get_set().get_uuid();
    }
    if (l_file_uuid.is_nil() || l_file_uuid != in_working_file.uuid_id_)
      FSys::software_flag_file(l_p, in_working_file.uuid_id_);
    in_working_file.name_ = in_working_file.path_.filename().generic_string();
    in_working_file.size_ = FSys::file_size(l_p);
    in_result->working_files_.emplace_back(in_working_file);
  } else {
    in_working_file = l_sql.get_by_uuid<working_file>(l_file_uuid);
  }
  using namespace sqlite_orm;
  if (l_sql.impl_->storage_any_.count<working_file_entity_link>(where(
          c(&working_file_entity_link::working_file_id_) == in_working_file.uuid_id_ &&
          c(&working_file_entity_link::entity_id_) == in_entt.uuid_id_
      )) == 0)
    in_result->working_file_entity_links_.emplace_back(
        working_file_entity_link{.working_file_id_ = in_working_file.uuid_id_, .entity_id_ = in_entt.uuid_id_}
    );
  if (l_sql.impl_->storage_any_.count<working_file_task_link>(where(
          c(&working_file_task_link::working_file_id_) == in_working_file.uuid_id_ &&
          c(&working_file_task_link::task_id_) == in_task.uuid_id_
      )) == 0)
    in_result->working_file_task_links_.emplace_back(
        working_file_task_link{.working_file_id_ = in_working_file.uuid_id_, .task_id_ = in_task.uuid_id_}
    );
}

boost::asio::awaitable<void> scan_result::install_async_sqlite() {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (!working_files_.empty()) co_await l_sql.install_range(&working_files_);
  if (!working_file_entity_links_.empty()) co_await l_sql.install_range(&working_file_entity_links_);
  if (!working_file_task_links_.empty()) co_await l_sql.install_range(&working_file_task_links_);
  co_return;
}

std::shared_ptr<scan_result> scan_task(const task& in_task) {
  static std::set<uuid> l_scan_uuids{
      task_type::get_character_id(),
      task_type::get_ground_model_id(),
      task_type::get_binding_id(),
      task_type::get_simulation_id(),
  };
  auto l_result = std::make_shared<scan_result>();
  if (!l_scan_uuids.contains(in_task.task_type_id_)) return l_result;  // 只处理特定类型的任务

  auto l_task_type_id = in_task.task_type_id_;
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_prj          = l_sql.get_by_uuid<project>(in_task.project_id_);
  auto l_entt         = l_sql.get_by_uuid<entity>(in_task.entity_id_);
  default_logger_raw()->info("扫描资产 {}", l_entt.name_);
  entity_asset_extend l_extend;
  if (auto l_entt_extend = l_sql.get_entity_asset_extend(l_entt.uuid_id_); !l_entt_extend.has_value()) {
    // 如果没有扩展数据, 则不进行扫描
    default_logger_raw()->warn("资产 {} 扩展数据不存在", l_entt.name_);
    return l_result;
  } else
    l_extend = l_entt_extend.value();

  auto l_l_working_files_list = l_sql.get_working_file_by_task(in_task.uuid_id_);

  if (!l_l_working_files_list.empty() && std::ranges::all_of(l_l_working_files_list, [&](const auto& l_file) {
        return !l_file.path_.empty() && FSys::exists(l_file.path_);
      }))
    return default_logger_raw()->info("资产 {} 已经存在, 不需要重新扫描", l_entt.name_),
           l_result;  // 如果所有的工作文件都存在, 则不需要重新扫描
  working_file l_maya_working_file{}, l_unreal_working_file{}, l_alembic_working_file{}, l_unreal_sk_working_file{};

  for (auto&& i : l_l_working_files_list) {
    if (i.software_type_ == software_enum::maya) {
      l_maya_working_file = i;
    } else if (i.software_type_ == software_enum::unreal_engine) {
      l_unreal_working_file = i;
    } else if (i.software_type_ == software_enum::alembic) {
      l_alembic_working_file = i;
    } else if (i.software_type_ == software_enum::unreal_engine_sk) {
      l_unreal_sk_working_file = i;
    }
  }

  if (l_task_type_id == task_type::get_character_id()) {
    if (l_maya_working_file.path_.empty() || !FSys::exists(l_maya_working_file.path_)) {
      l_maya_working_file.description_   = "maya模型文件";
      l_maya_working_file.path_          = scan_maya(l_prj, l_entt.entity_type_id_, l_extend);
      l_maya_working_file.software_type_ = software_enum::maya;
      scan_add_linked_data(l_maya_working_file, l_result, l_prj, l_entt, in_task);
    }
    if (l_unreal_sk_working_file.path_.empty() || !FSys::exists(l_unreal_sk_working_file.path_)) {
      l_unreal_sk_working_file.description_   = "UE SK文件";
      l_unreal_sk_working_file.path_          = scan_unreal_engine(l_prj, l_entt.entity_type_id_, l_extend);
      l_unreal_sk_working_file.software_type_ = software_enum::unreal_engine_sk;
      scan_add_linked_data(l_unreal_sk_working_file, l_result, l_prj, l_entt, in_task);
    }

  } else if (l_task_type_id == task_type::get_ground_model_id()) {
    if (l_unreal_working_file.path_.empty() || !FSys::exists(l_unreal_working_file.path_)) {
      l_unreal_working_file.description_   = "UE模型文件";
      l_unreal_working_file.path_          = scan_unreal_engine(l_prj, l_entt.entity_type_id_, l_extend);
      l_unreal_working_file.software_type_ = software_enum::unreal_engine;
      scan_add_linked_data(l_unreal_working_file, l_result, l_prj, l_entt, in_task);
    }
    if (l_alembic_working_file.path_.empty() || !FSys::exists(l_alembic_working_file.path_)) {
      l_alembic_working_file.description_   = "alembic地编模型文件";
      l_alembic_working_file.path_          = scan_alembic(l_prj, l_entt.entity_type_id_, l_extend);
      l_alembic_working_file.software_type_ = software_enum::alembic;
      scan_add_linked_data(l_alembic_working_file, l_result, l_prj, l_entt, in_task);
    }
    if (l_maya_working_file.path_.empty() || !FSys::exists(l_maya_working_file.path_)) {
      l_maya_working_file.description_   = "maya模型文件";
      l_maya_working_file.path_          = scan_maya(l_prj, l_entt.entity_type_id_, l_extend);
      l_maya_working_file.software_type_ = software_enum::maya;
      if (!l_maya_working_file.path_.empty() && FSys::exists(l_maya_working_file.path_))
        scan_add_linked_data(l_maya_working_file, l_result, l_prj, l_entt, in_task);
    }
    if (l_unreal_sk_working_file.path_.empty() || !FSys::exists(l_unreal_sk_working_file.path_)) {
      l_unreal_sk_working_file.description_   = "UE地编SK文件";
      l_unreal_sk_working_file.path_          = scan_scene_unreal_engine_sk(l_prj, l_extend);
      l_unreal_sk_working_file.software_type_ = software_enum::unreal_engine_sk;
      if (!l_unreal_sk_working_file.path_.empty() && FSys::exists(l_unreal_sk_working_file.path_))
        scan_add_linked_data(l_unreal_sk_working_file, l_result, l_prj, l_entt, in_task);
    }

  }

  else if (l_task_type_id == task_type::get_binding_id()) {
    if (l_maya_working_file.path_.empty() || !FSys::exists(l_maya_working_file.path_)) {
      l_maya_working_file.description_   = "绑定maya模型文件";
      l_maya_working_file.path_          = scan_rig_maya(l_prj, l_entt.entity_type_id_, l_extend);
      l_maya_working_file.software_type_ = software_enum::maya;
      scan_add_linked_data(l_maya_working_file, l_result, l_prj, l_entt, in_task);
    }

  } else if (l_task_type_id == task_type::get_simulation_id()) {
    if (l_maya_working_file.path_.empty() || !FSys::exists(l_maya_working_file.path_)) {
      l_maya_working_file.description_ = "模拟maya模型文件";
      using namespace sqlite_orm;
      auto l_tasks = l_sql.impl_->storage_any_.get_all<task>(
          where(c(&task::entity_id_) == in_task.entity_id_ && c(&task::task_type_id_) == task_type::get_binding_id())
      );
      if (l_tasks.empty()) return l_result;  // 没有绑定任务, 无法进行模拟
      auto l_work_file = l_sql.get_working_file_by_task(l_tasks.front().uuid_id_);
      if (l_work_file.empty()) return l_result;  // 没有绑定任务的工作文件, 无法进行模拟
      // 这里假设模拟的maya文件是绑定任务的maya文件
      l_maya_working_file.path_          = scan_sim_maya(l_prj, l_work_file.front());
      l_maya_working_file.software_type_ = software_enum::maya;
      scan_add_linked_data(l_maya_working_file, l_result, l_prj, l_entt, in_task);
    }
  }
  return l_result;
}
boost::asio::awaitable<std::shared_ptr<scan_result>> scan_task_async(const task& in_task) {
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<std::int64_t> l_delete_ids{};
  for (auto&& l_f : l_sql.get_working_file_by_task(in_task.uuid_id_))
    if (!l_f.path_.empty() && !FSys::exists(l_f.path_)) l_delete_ids.emplace_back(l_f.id_);
  if (!l_delete_ids.empty()) co_await l_sql.remove<working_file>(l_delete_ids);

  auto l_working_files = scan_task(in_task);
  co_await l_working_files->install_async_sqlite();
  co_return l_working_files;
}

}  // namespace doodle::scan_assets