//
// Created by TD on 2021/12/13.
//

#include "reference_comm.h"

#include <doodle_lib/lib_warp/entt_warp.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/metadata/project.h>

#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
#include <maya/MArgParser.h>
#include <maya/MAnimControl.h>

#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>

#include <magic_enum.hpp>

#define doodle_startTime "-st"
#define doodle_endTime "-et"
#define doodle_default_uuid "-u"
#define doodle_export_type "-ef"

#define doodle_export_type_long "-exportType"
#define doodle_default_uuid_long "-uuid"
#define doodle_startTime_long "-startTime"
#define doodle_endTime_long "-endTime"

namespace doodle::maya_plug {

MSyntax create_ref_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_default_uuid, doodle_default_uuid_long, MSyntax::kString);
  return syntax;
};
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
  return syntax;
}
MStatus create_ref_file_command::doIt(const MArgList& in_arg) {
  MStatus k_s;
  MArgParser k_prase{syntax(), in_arg, &k_s};
  entt::handle k_def_prj;
  if (k_prase.isFlagSet(doodle_default_uuid, &k_s)) {
    DOODLE_CHICK(k_s);
    string k_str = d_str{k_prase.flagArgumentString(doodle_default_uuid, 0, &k_s)};
    DOODLE_CHICK(k_s);
    auto k_def_uuid = boost::lexical_cast<uuid>(k_str);

    auto k_prj_view = g_reg()->view<project>();
    for (auto& k_e : k_prj_view) {
      auto k_h = make_handle(k_e);
      if (k_h.get<database>() == k_def_uuid) {
        k_def_prj = k_h;
        g_reg()->set<root_ref>(k_h);
      }
    }
  }

  DOODLE_LOG_INFO(
      "获得默认项目 {}", bool(k_def_prj));
  auto k_view = g_reg()->view<reference_file>();
  g_reg()->destroy(k_view.begin(), k_view.end());

  std::vector<entt::handle> l_list{};
  MFnReference k_ref_file{};
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_ref_file.setObject(refIter.thisNode());
    DOODLE_CHICK(k_s);
    auto k_obj = refIter.thisNode(&k_s);
    DOODLE_CHICK(k_s);
    auto k_h = make_handle();
    try {
      k_h.emplace<reference_file>().set_path(k_obj);
      DOODLE_CHICK(k_s);
      l_list.push_back(k_h);
    } catch (maya_error& err) {
      DOODLE_LOG_WARN("跳过无效的引用");
      k_h.destroy();
    }
  }
  return k_s;
}
MStatus ref_file_load_command::doIt(const MArgList& in_arg_list) {
  MStatus k_s{};
  chick_ctx<root_ref>();
  auto k_def_prj = g_reg()->ctx<root_ref>().root_handle();
  auto k_j_str   = maya_file_io::get_channel_date();
  std::vector<entt::entity> k_delete{};

  auto k_j = nlohmann::json::parse(k_j_str);
  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    auto l_i = make_handle(k_e);
    auto l_p = k_ref.path;
    if (k_j.contains(l_p)) {
      DOODLE_LOG_INFO("加载元数据 {}", l_p);
      entt_tool::load_comm<reference_file>(l_i, k_j.at(l_p));
    }
    if (!l_i.get<reference_file>().has_ref_project()) {
      l_i.patch<reference_file>([&](reference_file& in) {
        in.set_project(k_def_prj);
      });
    }
    try {
      if (k_j.contains(k_ref.get_unique_name()))
        k_ref.use_sim = k_j.at(k_ref.get_unique_name()).at("use_sim").get<bool>();
    } catch (nlohmann::json::exception& err) {
      DOODLE_LOG_ERROR(err.what());
    }
    if (!l_i.get<reference_file>().use_sim)
      k_delete.push_back(l_i);
  }
  g_reg()->destroy(k_delete.begin(), k_delete.end());

  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    if (!k_ref.replace_sim_assets_file()) {
      k_delete.push_back(k_e);
    } else {
      k_ref.generate_cloth_proxy();
    }
  }
  g_reg()->destroy(k_delete.begin(), k_delete.end());
  return k_s;
}
MStatus ref_file_sim_command::doIt(const MArgList& in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  MTime k_start{MAnimControl::minTime()};
  MTime k_end = MAnimControl::maxTime();
  /// \brief 在这里我们保存引用
  try {
    auto k_save_file = maya_file_io::work_path("ma");
    if (!FSys::exists(k_save_file)) {
      FSys::create_directories(k_save_file);
    }
    k_save_file /= maya_file_io::get_current_path().filename();
    k_s = MFileIO::saveAs(d_str{k_save_file.generic_string()}, nullptr, true);
    DOODLE_LOG_INFO("保存文件到 {}", k_save_file);
    DOODLE_CHICK(k_s);
  } catch (maya_error& error) {
    DOODLE_LOG_WARN("无法保存文件: {}", error);
  }

  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_startTime, 0, k_start);
    DOODLE_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_endTime, 0, k_end);
    DOODLE_CHICK(k_s);
  }
  DOODLE_LOG_INFO(
      "解算开始时间 {}  结束时间 {}  ",
      k_start.value(), k_end.value());

  for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
    k_ref.add_collision();
  }
  for (auto&& [k_e, k_qs] : g_reg()->view<qcloth_shape>().each()) {
    k_qs.set_cache_folder();
  }

  for (MTime k_t = k_start; k_t <= k_end; ++k_t) {
    for (auto&& [k_e, k_ref] : g_reg()->view<qcloth_shape>().each()) {
      MAnimControl::setCurrentTime(k_t);
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
  export_type k_export_type{};

  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_startTime, 0, k_start);
    DOODLE_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_endTime, 0, k_end);
    DOODLE_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_export_type, &k_s)) {
    DOODLE_CHICK(k_s);
    MString k_k_export_type_s{};
    k_s = k_prase.getFlagArgument(doodle_export_type, 0, k_k_export_type_s);
    DOODLE_CHICK(k_s);
    k_export_type = magic_enum::enum_cast<export_type>(d_str{k_k_export_type_s}.str()).value_or(export_type::abc);
  }

  DOODLE_LOG_INFO(
      "导出开始时间 {}  结束时间 {} 导出类型 {} ",
      k_start.value(), k_end.value(), magic_enum::enum_name(k_export_type));

  for (auto&& [k_e, k_r] : g_reg()->view<reference_file>().each()) {
    switch (k_export_type) {
      case export_type::abc:
        k_r.export_abc(k_start, k_end);
        break;
      case export_type::fbx:
        k_r.export_fbx(k_start, k_end);
        break;
      default:
        throw doodle_error{"未知类型"};
        break;
    }
  }
  return k_s;
}
}  // namespace doodle::maya_plug
#undef doodle_startTime
#undef doodle_endTime
#undef doodle_default_uuid

#undef doodle_default_uuid_long
#undef doodle_startTime_long
#undef doodle_endTime_long
