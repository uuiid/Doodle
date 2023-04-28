//
// Created by TD on 2021/12/13.
//
#include "reference_comm.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/app/app_command.h>

#include <boost/asio/use_future.hpp>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/fmt/fmt_select_list.h>

#include <magic_enum.hpp>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>

namespace doodle::maya_plug {
namespace {
constexpr const char doodle_startTime[]              = "-st";
constexpr const char doodle_endTime[]                = "-et";
constexpr const char doodle_project_path[]           = "-pr";
constexpr const char doodle_export_type[]            = "-ef";
constexpr const char doodle_export_use_select[]      = "-s";
constexpr const char doodle_export_force[]           = "-f";

constexpr const char doodle_export_type_long[]       = "-exportType";
constexpr const char doodle_project_path_long[]      = "-project";
constexpr const char doodle_startTime_long[]         = "-startTime";
constexpr const char doodle_endTime_long[]           = "-endTime";
constexpr const char doodle_export_use_select_long[] = "-select";
constexpr const char doodle_export_force_long[]      = "-force";

};  // namespace
namespace create_ref_file_command_ns {
constexpr const char face_all[]     = "-a";
constexpr const char face_all_log[] = "-allload";
MSyntax syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(face_all, face_all_log, MSyntax::kBoolean);
  return l_syntax;
}

}  // namespace create_ref_file_command_ns

MSyntax ref_file_sim_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_startTime, doodle_startTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_endTime, doodle_endTime_long, MSyntax::kTime);
  return syntax;
}
MSyntax ref_file_export_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_startTime, doodle_startTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_endTime, doodle_endTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_export_type, doodle_export_type_long, MSyntax::kString);
  syntax.addFlag(doodle_export_use_select, doodle_export_use_select_long, MSyntax::kBoolean);
  syntax.addFlag(doodle_export_force, doodle_export_force_long, MSyntax::kBoolean);
  return syntax;
}

MSyntax load_project_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_project_path, doodle_project_path_long, MSyntax::kString);
  return syntax;
}
MSyntax set_cloth_cache_path_syntax() {
  MSyntax l_syntax{};
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}

MStatus ref_file_load_command::doIt(const MArgList& in_arg_list) {
  MStatus k_s{};
  auto k_j_str = maya_file_io::get_channel_date();
  std::vector<entt::entity> k_delete{};

  nlohmann::json k_j{};
  try {
    k_j = nlohmann::json::parse(k_j_str);
  } catch (const nlohmann::json::exception& error) {
    DOODLE_LOG_WARN("解析元数据错误 {}, 取消解析元数据，使用默认元数据", boost::diagnostic_information(error));
  }
  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    auto l_i = make_handle(k_e);
    auto l_p = k_ref.path;
    if (k_j.contains(l_p)) {
      DOODLE_LOG_INFO("加载元数据 {}", l_p);
      entt_tool::load_comm<reference_file, sim_cover_attr>(l_i, k_j.at(l_p));
    } else {
      l_i.get<reference_file>().use_sim = true;
    }

    if (!l_i.get<reference_file>().use_sim) {
      DOODLE_LOG_INFO("引用文件{}不解算", l_i.get<reference_file>().path);
      k_delete.push_back(l_i);
    }
  }
  g_reg()->destroy(k_delete.begin(), k_delete.end());

  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    if (k_ref.replace_sim_assets_file()) {
    } else {
      k_delete.push_back(k_e);
    }
  }
  g_reg()->destroy(k_delete.begin(), k_delete.end());
  return k_s;
}
MStatus ref_file_sim_command::doIt(const MArgList& in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  auto k_start = MAnimControl::minTime();
  auto k_end   = MAnimControl::maxTime();

  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_startTime, 0, k_start);
    DOODLE_MAYA_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_endTime, 0, k_end);
    DOODLE_MAYA_CHICK(k_s);
  }
  DOODLE_LOG_INFO("解算开始时间 {}  结束时间 {}  ", k_start.value(), k_end.value());

  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    DOODLE_LOG_INFO("引用文件{}发现需要设置解算碰撞体", k_ref.path);
    /// \brief 生成需要的 布料实体
    qcloth_shape::create(make_handle(k_e));
    /// \brief 添加碰撞体
    k_ref.add_collision();
    /// \brief 更新ql初始化状态
    k_ref.qlUpdateInitialPose();
    /// \brief 更新元配置数据
    DOODLE_LOG_INFO("开始测试 {} 覆盖属性", k_ref.path);
    sim_cover_attr::cover_qcloth_attr(make_handle(k_e));
  }
  for (auto&& [k_e, k_qs] : g_reg()->view<qcloth_shape>().each()) {
    /// @brief 添加风场
    k_qs.add_field();

    DOODLE_LOG_INFO("开始设置解算布料的缓存文件夹");
    k_qs.set_cache_folder();
  }

  /// \brief 在这里我们保存引用
  auto k_save_file = maya_file_io::work_path("ma");
  if (!FSys::exists(k_save_file)) {
    FSys::create_directories(k_save_file);
  }
  k_save_file /= maya_file_io::get_current_path().filename();
  try {
    k_s = MFileIO::saveAs(d_str{k_save_file.generic_string()}, nullptr, true);
    DOODLE_LOG_INFO("保存文件到 {}", k_save_file);
    DOODLE_MAYA_CHICK(k_s);
  } catch (const maya_error& error) {
    DOODLE_LOG_WARN("无法保存文件 {} : {}", k_save_file, error);
  }

  for (MTime k_t = k_start; k_t <= k_end; ++k_t) {
    MAnimControl::setCurrentTime(k_t);
    for (auto&& [k_e, k_ref] : g_reg()->view<qcloth_shape>().each()) {
      k_ref.create_cache();
    }
  }
  return k_s;
}
MStatus ref_file_export_command::doIt(const MArgList& in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  MTime k_start{MAnimControl::minTime()};
  MTime k_end = MAnimControl::maxTime();
  bool use_select{false};
  bool is_force{false};
  reference_file::export_type k_export_type{};

  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_startTime, 0, k_start);
    DOODLE_MAYA_CHICK(k_s);
  } else {
    k_start = MTime{
        boost::numeric_cast<std::double_t>(g_reg()->ctx().get<project_config::base_config>().export_anim_time),
        MTime::uiUnit()};
  }
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_endTime, 0, k_end);
    DOODLE_MAYA_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_export_type, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MString k_k_export_type_s{};
    k_s = k_prase.getFlagArgument(doodle_export_type, 0, k_k_export_type_s);
    DOODLE_MAYA_CHICK(k_s);
    k_export_type = magic_enum::enum_cast<reference_file::export_type>(d_str{k_k_export_type_s}.str()).value();
  }
  if (k_prase.isFlagSet(doodle_export_use_select, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_export_use_select, 0, use_select);
    DOODLE_MAYA_CHICK(k_s);
  }

  if (k_prase.isFlagSet(doodle_export_force, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_export_force, 0, is_force);
    DOODLE_MAYA_CHICK(k_s);
    if (!use_select) {
      DOODLE_LOG_ERROR("错误, 强制导出时必须使用选择标志并选中导出物体");
      return {MStatus::kFailure};
    }
  }

  DOODLE_LOG_INFO(
      "导出开始时间 {}  结束时间 {} 导出类型 {} ", k_start.value(), k_end.value(), magic_enum::enum_name(k_export_type)
  );

  if (is_force) {
    DOODLE_LOG_INFO("开始使用交互式导出");
    MSelectionList k_select{};
    k_s = MGlobal::getActiveSelectionList(k_select);
    for (auto&& [k_e, k_r] : g_reg()->view<reference_file>().each()) {
      if (k_r.has_node(k_select)) {
        reference_file::export_arg l_export_arg{k_export_type, k_start, k_end};
        k_r.export_file_select(l_export_arg, k_select);
        return k_s;
      }
    }
  }
  if (use_select) {
    MSelectionList k_select{};
    k_s = MGlobal::getActiveSelectionList(k_select);
    DOODLE_LOG_INFO("获取选中物体 {}", k_select);
    for (auto&& [k_e, k_r] : g_reg()->view<reference_file>().each()) {
      if (k_r.has_node(k_select)) {
        DOODLE_LOG_INFO("开始导出 {}", k_r.path);
        reference_file::export_arg l_export_arg{k_export_type, k_start, k_end};
        k_r.export_file(l_export_arg);
      }
    }
  } else {
    DOODLE_LOG_INFO("全部的引用文件导出");
    for (auto&& [k_e, k_r] : g_reg()->view<reference_file>().each()) {
      DOODLE_LOG_INFO("开始导出 {}", k_r.path);
      reference_file::export_arg l_export_arg{k_export_type, k_start, k_end};
      k_r.export_file(l_export_arg);
    }
  }

  return k_s;
}

MStatus load_project::doIt(const MArgList& in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  FSys::path k_path{};

  if (k_prase.isFlagSet(doodle_project_path, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MString k_path_M{};
    k_s = k_prase.getFlagArgument(doodle_project_path, 0, k_path_M);
    DOODLE_LOG_INFO("开始打开项目 {}", k_path_M);
    if (k_path_M.numChars() > 0) {
      k_path = k_path_M.asUTF8();
      bool run{true};
      doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->async_open(
          k_path,
          [k_path, l_run = &run](bsys::error_code) -> void {
            DOODLE_LOG_INFO("完成打开项目 {}", k_path);
            *l_run = false;
          }
      );
      if (MGlobal::mayaState(&k_s) != MGlobal::kInteractive) {
        while (run) g_io_context().poll_one();
      }
    }
  }
  return k_s;
}

MStatus set_cloth_cache_path::doIt(const MArgList& in_list) {
  MStatus l_status{};
  MArgDatabase k_prase{syntax(), in_list, &l_status};
  MSelectionList l_list{};
  DOODLE_MAYA_CHICK(k_prase.getObjects(l_list));

  MObject l_object{};
  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    DOODLE_LOG_INFO("引用文件{}被发现需要设置解算碰撞体", k_ref.path);
    /// \brief 生成需要的 布料实体
    if (!l_list.isEmpty())
      for (auto l_i = MItSelectionList{l_list}; !l_i.isDone(); l_i.next()) {
        DOODLE_MAYA_CHICK(l_i.getDependNode(l_object));
        if (k_ref.has_node(l_object)) qcloth_shape::create(make_handle(k_e));
      }
    else
      qcloth_shape::create(make_handle(k_e));
  }
  for (auto&& [k_e, k_qs] : g_reg()->view<qcloth_shape>().each()) {
    DOODLE_LOG_INFO("开始设置解算布料的缓存文件夹");
    k_qs.set_cache_folder(g_reg()->ctx().get<user::current_user>().user_name_attr());
  }
  return l_status;
}

}  // namespace doodle::maya_plug
