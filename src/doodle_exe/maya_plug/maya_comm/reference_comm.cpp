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

#define doodle_startTime "-st"
#define doodle_endTime "-et"
#define doodle_default_uuid "-u"

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
  auto k_j_str = maya_file_io::get_channel_date();
  if (k_j_str.empty() && !k_def_prj) {
    DOODLE_LOG_ERROR("找不到默认配置， 并且文件中也找不到解算元数据");
    return MStatus::kFailure;
  }
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
  auto k_j = nlohmann::json::parse(k_j_str);
  for (auto& l_i : l_list) {
    auto& k_ref = l_i.get<reference_file>();
    auto l_p    = k_ref.path;
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
      l_i.destroy();
  }
  return k_s;
}
MStatus ref_file_load_command::doIt(const MArgList& in_arg_list) {
  MStatus k_s{};
  auto k_view = g_reg()->view<reference_file>();
  std::vector<entt::entity> k_delete{};
  for (auto&& [k_e, k_ref] : k_view.each()) {
    if (!k_ref.replace_sim_assets_file()) {
      k_delete.push_back(k_e);
    } else {
      k_ref.chick_cloth_proxy();
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
    k_ref.set_cloth_cache_dir();
    k_ref.add_collision();
  }

  for (MTime k_t = k_start; k_t <= k_end; ++k_t) {
    for (auto&& [k_e, k_ref] : g_reg()->view<reference_file>().each()) {
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
      "导出开始时间 {}  结束时间 {}  ",
      k_start.value(), k_end.value());

  for (auto&& [k_e, k_r] : g_reg()->view<reference_file>().each()) {
    k_r.export_abc(k_start, k_end);
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
