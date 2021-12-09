//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_lib/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/metadata/project.h>

#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MAnimControl.h>
#include <maya/MArgList.h>

#include <maya_plug/command/reference_file.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/maya_plug_fwd.h>

#include <nlohmann/json.hpp>

namespace doodle::maya_plug {

namespace reference_attr {

bool data::operator==(const data& in_rhs) const {
  return path == in_rhs.path;
}
bool data::operator!=(const data& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace reference_attr

reference_attr_setting::reference_attr_setting()
    : command_base(),
      p_handle() {
  p_name          = "引用编辑";
  p_show_str      = make_imgui_name(this,
                                    "解析引用",
                                    "引用",
                                    "设置场景",
                                    "保存");
  auto k_ref_view = g_reg()->view<reference_file>();
  std::transform(k_ref_view.begin(), k_ref_view.end(),
                 std::back_inserter(p_handle),
                 [](auto& in_e) {
                   return make_handle(in_e);
                 });
}

bool reference_attr_setting::chick_channel() const {
  MStatus k_status{};
  adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
  DOODLE_CHICK(k_status);
  auto k_channel = k_meta.channel("doodle_sim_json");
  if (!k_channel.dataStream("json_stream")) {
    auto k_s = adsk::Data::Structure::create();              // 添加结构
    k_s->setName("json_structure");                          // 设置结构名称
    k_s->addMember(adsk::Data::Member::kString, 1, "json");  // 添加结构成员
    adsk::Data::Structure::registerStructure(*k_s);          // 注册结构

    adsk::Data::Stream k_stream{*k_s, "json_stream"};
    k_channel.setDataStream(k_stream);  // 设置流
    MFileIO::setMetadata(k_meta);
  }
  return true;
}
bool reference_attr_setting::replace_channel_date(const string& in_string) const {
  chick_channel();
  MStatus k_status{};
  adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
  DOODLE_CHICK(k_status);
  auto k_json   = k_meta.channel("doodle_sim_json");  /// 获得元数据通道
  auto k_stream = k_json.dataStream("json_stream");   /// 获得元数据流

  adsk::Data::Handle k_h{k_stream->structure()};

  string str_err{};
  DOODLE_LOG_INFO(in_string);
  if (k_h.fromStr(in_string, 0, str_err) != 0)
    DOODLE_LOG_ERROR(str_err);
  k_stream->setElement(0, k_h);
  MFileIO::setMetadata(k_meta);

  adsk::Debug::Print k_p{std::cout};
  decltype(k_meta)::Debug(&k_meta, k_p);
  k_p.endSection();
  return true;
}
string reference_attr_setting::get_channel_date() {
  MStatus k_status{};
  adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
  DOODLE_CHICK(k_status);
  auto k_channel = k_meta.channel("doodle_sim_json");
  if (!k_channel.empty()) {
    auto k_stream = k_channel.dataStream("json_stream");
    adsk::Data::Handle k_h{k_stream->element(0)};
    if (!k_h.hasData())
      return {};
    return k_h.str(0);
  }
  return {};
}
bool reference_attr_setting::get_file_info() {
  adsk::Debug::Print k_p{std::cout};
  MStatus k_status{};
  DOODLE_CHICK(k_status);
  for (auto& k_h : p_handle) {
    k_h.destroy();
  }
  p_handle.clear();

  MStatus k_s{};
  MFnReference k_ref_file{};
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_ref_file.setObject(refIter.thisNode());
    DOODLE_CHICK(k_s);
    auto k_obj = refIter.thisNode(&k_s);
    DOODLE_CHICK(k_s);
    auto k_h = make_handle();
    try {
      k_h.emplace<reference_file>(g_reg()->ctx<root_ref>().root_handle(), k_obj);
      DOODLE_CHICK(k_s);
      p_handle.push_back(k_h);
    } catch (maya_error& err) {
      DOODLE_LOG_WARN("跳过无效的引用");
      k_h.destroy();
    }
  }

  auto k_j_str = get_channel_date();
  if (k_j_str.empty())
    return true;
  auto k_j = nlohmann::json::parse(k_j_str);

  for (auto& l_i : p_handle) {
    auto& k_ref = l_i.get<reference_file>();
    auto l_p    = k_ref.path;
    if (k_j.contains(l_p))
      entt_tool::load_comm<reference_file>(l_i, k_j.at(l_p));
    l_i.get<reference_file>().init_show_name();
  }
  return true;
}

bool reference_attr_setting::render() {
  if (imgui::Button(p_show_str["解析引用"].c_str())) {
    get_file_info();
  }
  MStatus k_s{};
  MSelectionList l_select{};
  auto k_ref_view = g_reg()->view<reference_file>();

  for (auto k_e : k_ref_view) {
    auto& k_ref = k_ref_view.get<reference_file>(k_e);
    dear::TreeNode{k_ref.path.c_str()} && [&]() {
      imgui::Checkbox("解算", &(k_ref.use_sim));
      if (!k_ref.use_sim)
        return;

      imgui::Checkbox("高精度配置", &(k_ref.high_speed_sim));
      if (imgui::Button("替换")) {
        k_s = MGlobal::getActiveSelectionList(l_select);
        DOODLE_CHICK(k_s);
        k_ref.set_collision_model(l_select);
      }
      if (imgui::Button("获取")) {
        MGlobal::setActiveSelectionList(k_ref.get_collision_model());
      }
      dear::Text("解算碰撞");
      for (const auto& k_f : k_ref.collision_model_show_str)
        dear::Text(k_f);

      // if (imgui::Button("test")) {
      //   try {
      //     k_ref.replace_sim_assets_file();
      //   } catch (const maya_error& e) {
      //     std::cerr << e.what() << '\n';
      //   }
      // }
    };
  }

  if (imgui::Button(p_show_str["保存"].c_str())) {
    chick_channel();
    nlohmann::json k_j{};
    for (auto& k : p_handle) {
      entt_tool::save_comm<reference_file>(k, k_j[k.get<reference_file>().path]);
    }
    replace_channel_date(k_j.dump());
  }

  return true;
}

#define doodle_startTime "-st"
#define doodle_endTime "-et"
#define doodle_default_uuid "-u"

#define doodle_default_uuid_long "-uuid"
#define doodle_startTime_long "-startTime"
#define doodle_endTime_long "-endTime"

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
  auto k_j_str = reference_attr_setting::get_channel_date();
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
