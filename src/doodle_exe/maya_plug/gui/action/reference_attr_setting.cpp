//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_lib/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
#include <maya_plug/command/reference_file.h>
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

bool reference_attr_setting::add_channel() const {
  MStatus k_status{};
  adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
  CHECK_MSTATUS_AND_RETURN(k_status, false);
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

bool reference_attr_setting::get_file_info() {
  adsk::Debug::Print k_p{std::cout};
  MStringArray file_list;
  MStatus k_status{};
  k_status = MFileIO::getReferences(file_list);
  CHECK_MSTATUS_AND_RETURN(k_status, false);
  for (auto& k_h : p_handle) {
    k_h.destroy();
  }
  p_handle.clear();

#if MAYA_API_VERSION > 20180000 && MAYA_API_VERSION < 20190000
  for (auto i = 0; i < file_list.length(); ++i) {
    auto k_r = make_handle();
    k_r.emplace<reference_file>(g_reg()->ctx<root_ref>().root_handle(), std::string{file_list[i].asUTF8()});
    p_handle.push_back(k_r);
  }
#elif MAYA_API_VERSION > 20190000 && MAYA_API_VERSION < 20200000
  for (auto& in : file_list) {
    auto k_r = make_handle();
    k_r.emplace<reference_file>(g_reg()->ctx<root_ref>().root_handle(), std::string{in.asUTF8()});
    p_handle.push_back(k_r);
  }
#elif MAYA_API_VERSION > 20200000
  std::transform(file_list.begin(),
                 file_list.end(),
                 std::back_inserter(p_handle),
                 [](const MString& in) -> entt::handle {
                   auto k_r = make_handle();
                   k_r.emplace<reference_file>(g_reg()->ctx<root_ref>().root_handle(), std::string{in.asUTF8()});
                   return k_r;
                 });

#endif
  add_channel();

  adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
  CHECK_MSTATUS_AND_RETURN(k_status, false);
  auto k_channel = k_meta.channel("doodle_sim_json");
  if (!k_channel.empty()) {
    auto k_stream = k_channel.dataStream("json_stream");
    adsk::Data::Handle k_h{k_stream->element(0)};
    if (!k_h.hasData())
      return false;
    auto k_str = k_h.str(0);
    if (k_str.empty())
      return false;

    auto k_j = nlohmann::json::parse(k_h.str(0));

    for (auto& l_i : p_handle) {
      auto l_p = l_i.get<reference_file>().path;
      if (k_j.contains(l_p))
        entt_tool::load_comm<reference_file>(l_i, k_j.at(l_p));
    }

    


  }
  decltype(k_meta)::Debug(&k_meta, k_p);
  k_p.endSection();
  return true;
}

bool reference_attr_setting::render() {
  if (imgui::Button(p_show_str["解析引用"].c_str())) {
    get_file_info();
  }

  auto k_ref_view = g_reg()->view<reference_file>();
  for (auto k_e : k_ref_view) {
    auto& k_ref = k_ref_view.get<reference_file>(k_e);
    dear::TreeNode{k_ref.path.c_str()} && [&]() {
      imgui::Checkbox("解算", &(k_ref.use_sim));
      if (!k_ref.use_sim)
        return;

      imgui::Checkbox("高精度配置", &(k_ref.high_speed_sim));
      if (imgui::Button("替换")) {
        add_collision(make_handle(k_e));
      }
      if (imgui::Button("获取")) {
        get_collision(make_handle(k_e));
      }
      dear::Text("解算碰撞");
      for (const auto& k_f : p_names)
        dear::Text(k_f);
    };
  }

  if (imgui::Button(p_show_str["保存"].c_str())) {
    add_channel();
    nlohmann::json k_j{};
    for (auto& k : p_handle) {
      entt_tool::save_comm<reference_file>(k, k_j[k.get<reference_file>().path]);
    }
    // std::transform(k_ref_view.begin(), k_ref_view.end(),
    //                std::back_inserter(k_j),
    //                [&](auto& in_e) -> nlohmann::json::object_t::value_type {
    //                  auto& k_ref_file = k_ref_view.get<reference_file>(in_e);
    //                  return {k_ref_file.path, k_ref_file};
    //                });

    MStatus k_status{};
    /// 获取文件元数据
    /// 转换元数据
    adsk::Data::Associations k_meta{MFileIO::metadata(&k_status)};
    CHECK_MSTATUS_AND_RETURN(k_status, false);
    auto k_json   = k_meta.channel("doodle_sim_json");  /// 获得元数据通道
    auto k_stream = k_json.dataStream("json_stream");   /// 获得元数据流

    adsk::Data::Handle k_h{k_stream->structure()};

    string str_err{};
    auto str = k_j.dump();
    DOODLE_LOG_INFO(str);
    k_h.fromStr(str, 0, str_err);
    DOODLE_LOG_ERROR(str_err);
    k_stream->setElement(0, k_h);
    MFileIO::setMetadata(k_meta);

    adsk::Debug::Print k_p{std::cout};
    decltype(k_meta)::Debug(&k_meta, k_p);
    k_p.endSection();
  }
  return true;
}
bool reference_attr_setting::add_collision(const entt::handle& in_ref) {
  if (!in_ref.any_of<reference_file>())
    throw component_error{"缺失组件"};

  auto& k_ref = in_ref.get<reference_file>();

  MStatus k_s{};
  k_s = MGlobal::getActiveSelectionList(p_select);
  DOODLE_CHICK(k_s);
  if (p_select.length() > 30) {
    MString k_str{};
    k_str.setUTF8("太多的选择");
    MGlobal::displayWarning(k_str);
    return false;
  }

  MDagPath l_path{};
  MFnDagNode l_node{};
  k_ref.collision_model.clear();
  p_names.clear();
  for (MItSelectionList l_it{p_select, MFn::Type::kMesh, &k_s};
       !l_it.isDone(&k_s);
       l_it.next()) {
    DOODLE_CHICK(k_s);
    k_s = l_it.getDagPath(l_path);
    DOODLE_CHICK(k_s);
    auto k_obj = l_path.transform(&k_s);
    DOODLE_CHICK(k_s);
    k_s = l_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
    p_names.emplace_back(l_node.name(&k_s).asUTF8());
    DOODLE_CHICK(k_s);

    k_ref.collision_model.emplace_back(l_node.uuid(&k_s).asString().asUTF8());
    DOODLE_CHICK(k_s);
  }

  return true;
}
bool reference_attr_setting::get_collision(const entt::handle& in_ref) const {
  MStatus k_s{};
  k_s = MGlobal::setActiveSelectionList(p_select);
  DOODLE_CHICK(k_s);
  return true;
}

}  // namespace doodle::maya_plug
