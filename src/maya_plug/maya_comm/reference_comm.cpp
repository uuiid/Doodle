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

MSyntax set_cloth_cache_path_syntax() {
  MSyntax l_syntax{};
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}

MStatus set_cloth_cache_path::doIt(const MArgList& in_list) {
  MStatus l_status{};
  // MArgDatabase k_prase{syntax(), in_list, &l_status};
  // MSelectionList l_list{};
  // DOODLE_MAYA_CHICK(k_prase.getObjects(l_list));

  // MObject l_object{};
  // for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
  //   DOODLE_LOG_INFO("引用文件{}被发现需要设置解算碰撞体", k_ref.path);
  //   /// \brief 生成需要的 布料实体
  //   if (!l_list.isEmpty())
  //     for (auto l_i = MItSelectionList{l_list}; !l_i.isDone(); l_i.next()) {
  //       DOODLE_MAYA_CHICK(l_i.getDependNode(l_object));
  //       if (k_ref.has_node(l_object)) qcloth_shape::create(make_handle(k_e));
  //     }
  //   else
  //     qcloth_shape::create(make_handle(k_e));
  // }
  // for (auto&& [k_e, k_qs] : g_reg()->view<qcloth_shape>().each()) {
  //   DOODLE_LOG_INFO("开始设置解算布料的缓存文件夹");
  //   k_qs.set_cache_folder(g_reg()->ctx().get<user::current_user>().user_name_attr());
  // }
  return l_status;
}

}  // namespace doodle::maya_plug
