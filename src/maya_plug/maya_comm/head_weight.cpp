//
// Created by TD on 25-8-28.
//

#include "head_weight.h"

#include <Eigen/Dense>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>
namespace doodle::maya_plug {

namespace {
// 对称的骨骼
struct sym_bone {
  std::vector<MDagPath> left_bones_;
  std::vector<MDagPath> right_bones_;
};
}  // namespace

class head_weight::impl {
 public:
  impl()  = default;
  ~impl() = default;
  // 头部网格
  MDagPath head_mesh_path_;

  std::vector<MDagPath> all_joint_list_;

  // 头部骨骼
  MDagPath M_Head_;
  MDagPath M_HeadBt_;
  MDagPath M_HeadNeck_;
  MDagPath M_HeadTop_;
  MDagPath M_Jaw_;
  MDagPath M_JawA_;
  MDagPath M_JawUpA_;
  MDagPath M_LipMove_;
  MDagPath M_LoLip1_;
  MDagPath M_MidBrow_;
  MDagPath M_Nose_;
  MDagPath M_NoseTip_;
  MDagPath M_UpLip1_;
  MDagPath M_UpLipEndA_;
  sym_bone Brow_;
  sym_bone BrowSec_;
  sym_bone Cheek_;
  sym_bone Ear_;
  sym_bone Jaw_;
  sym_bone JawUp_;
  sym_bone LoLid_;
  sym_bone LoLip_;
  sym_bone LoRing_;
  sym_bone MidBrow_;
  sym_bone NasolabialFold_;
  sym_bone NoseWing_;
  sym_bone Nostril_;
  sym_bone UpLid_;
  sym_bone UpLip_;
  sym_bone UpperCheek_;
  sym_bone UpRing_;

  static auto g_attribute_name_to_joint_map() {
    static std::map<std::string, decltype(&impl::M_Head_)> s_map{};
    s_map["M_Head"]      = &impl::M_Head_;
    s_map["M_HeadBt"]    = &impl::M_HeadBt_;
    s_map["M_HeadNeck"]  = &impl::M_HeadNeck_;
    s_map["M_HeadTop"]   = &impl::M_HeadTop_;
    s_map["M_Jaw"]       = &impl::M_Jaw_;
    s_map["M_JawA"]      = &impl::M_JawA_;
    s_map["M_JawUpA"]    = &impl::M_JawUpA_;
    s_map["M_LipMove"]   = &impl::M_LipMove_;
    s_map["M_LoLip1"]    = &impl::M_LoLip1_;
    s_map["M_MidBrow"]   = &impl::M_MidBrow_;
    s_map["M_Nose"]      = &impl::M_Nose_;
    s_map["M_NoseTip"]   = &impl::M_NoseTip_;
    s_map["M_UpLip1"]    = &impl::M_UpLip1_;
    s_map["M_UpLipEndA"] = &impl::M_UpLipEndA_;
    return s_map;
  }
  static auto g_sym_attribute_name_to_joint_map() {
    static std::map<std::string, decltype(&impl::Brow_)> s_map{};
    s_map["Brow"]           = &impl::Brow_;
    s_map["BrowSec"]        = &impl::BrowSec_;
    s_map["Cheek"]          = &impl::Cheek_;
    s_map["Ear"]            = &impl::Ear_;
    s_map["Jaw"]            = &impl::Jaw_;
    s_map["JawUp"]          = &impl::JawUp_;
    s_map["LoLid"]          = &impl::LoLid_;
    s_map["LoLip"]          = &impl::LoLip_;
    s_map["LoRing"]         = &impl::LoRing_;
    s_map["MidBrow"]        = &impl::MidBrow_;
    s_map["NasolabialFold"] = &impl::NasolabialFold_;
    s_map["NoseWing"]       = &impl::NoseWing_;
    s_map["Nostril"]        = &impl::Nostril_;
    s_map["UpLid"]          = &impl::UpLid_;
    s_map["UpLip"]          = &impl::UpLip_;
    s_map["UpperCheek"]     = &impl::UpperCheek_;
    s_map["UpRing"]         = &impl::UpRing_;
    return s_map;
  }
  static bool is_sym_bone(const std::string_view& in_bone, const std::string_view& in_check_name) {
    // 以 L_ 或者 R_ 开头的骨骼都是对称骨骼
    if (!(in_bone.starts_with("L_") || in_bone.starts_with("R_"))) {
      return false;
    }
    auto l_name = in_bone.substr(2);
    if (l_name != in_check_name) return false;

    // 继续检查后面, 必须是 A-Z 0-9 下划线结尾
    l_name = l_name.substr(in_check_name.length());
    if (l_name.length() < 3) return false;
    bool is_number = l_name.front() >= '0' && l_name.front() <= '9';
    bool is_letter = l_name.front() >= 'A' && l_name.front() <= 'Z';
    if (!is_number && !is_letter) return false;
    if (is_letter && l_name[1] != '_') return false;

    if (is_number) {
      // 双数字
      if (l_name[1] != '_' && l_name[1] >= '0' && l_name[1] <= '9') {
        if (l_name.length() < 4) return false;
        if (l_name[2] != '_') return false;
      }
    }
    return true;
  }
};

MSyntax head_weight_syntax() {
  MSyntax l_syntax;
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::kSelectionList, 1);
  return l_syntax;
}

head_weight::head_weight() : p_impl(std::make_unique<impl>()) {}
head_weight::~head_weight() = default;

MStatus head_weight::doIt(const MArgList& args) {
  MStatus l_s{};
  MArgDatabase l_arg_db(syntax(), args);
  MSelectionList l_sel_list{};
  CHECK_MSTATUS_AND_RETURN_IT(l_arg_db.getObjects(l_sel_list));
  if (l_sel_list.length() == 0) return MGlobal::displayError("请至少选择一个头部模型"), MStatus::kFailure;

  CHECK_MSTATUS_AND_RETURN_IT(l_sel_list.getDagPath(0, p_impl->head_mesh_path_));
  if (!p_impl->head_mesh_path_.hasFn(MFn::kMesh))
    return MGlobal::displayError("请选择一个头部网格模型"), MStatus::kFailure;

  auto l_mesh_shape = p_impl->head_mesh_path_;
  CHECK_MSTATUS_AND_RETURN_IT(l_mesh_shape.extendToShape());
  auto l_mesh_shape_node = l_mesh_shape.node(&l_s);
  CHECK_MSTATUS_AND_RETURN_IT(l_s);
  MObject l_skin_cluster{};
  /// 寻找皮肤簇
  for (MItDependencyGraph i{l_mesh_shape_node, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream};
       !i.isDone(); i.next()) {
    l_skin_cluster = i.currentItem(&l_s);
    CHECK_MSTATUS_AND_RETURN_IT(l_s);
  }
  if (l_skin_cluster.isNull()) return MGlobal::displayError("头部网格没有绑定皮肤"), MStatus::kFailure;
  MFnSkinCluster l_skin_cluster_fn{l_skin_cluster, &l_s};
  CHECK_MSTATUS_AND_RETURN_IT(l_s);
  MDagPathArray l_influence_objects{};
  auto l_joint_count = l_skin_cluster_fn.influenceObjects(l_influence_objects, &l_s);
  CHECK_MSTATUS_AND_RETURN_IT(l_s);
  if (l_joint_count == 0) return MGlobal::displayError("头部网格绑定的皮肤没有影响骨骼"), MStatus::kFailure;
  p_impl->all_joint_list_.reserve(l_joint_count);
  for (auto i = 0; i < l_joint_count; ++i) p_impl->all_joint_list_.emplace_back(l_influence_objects[i]);
  /// 载入对应骨骼
  for (const auto& [l_name, l_member_ptr] : impl::g_attribute_name_to_joint_map()) {
    bool l_found = false;
    for (const auto& l_joint : p_impl->all_joint_list_) {
      if (get_node_name(l_joint).starts_with(l_name)) {
        (*p_impl).*l_member_ptr = l_joint;
        l_found                 = true;
        break;
      }
    }
    if (!l_found) return MGlobal::displayError(fmt::format("未找到头部骨骼 {}", l_name).c_str()), MStatus::kFailure;
  }
  for (const auto& [l_name, l_member_ptr] : impl::g_sym_attribute_name_to_joint_map()) {
    bool l_found_left  = false;
    bool l_found_right = false;
    for (const auto& l_joint : p_impl->all_joint_list_) {
      auto l_bone_name = get_node_name(l_joint);
      if (!impl::is_sym_bone(l_bone_name, l_name)) continue;
      if (l_bone_name.front() == 'L') {
        ((*p_impl).*l_member_ptr).left_bones_.emplace_back(l_joint);
        l_found_left = true;
      } else if (l_bone_name.front() == 'R') {
        ((*p_impl).*l_member_ptr).right_bones_.emplace_back(l_joint);
        l_found_right = true;
      }
    }
    if (!l_found_left)
      return MGlobal::displayError(fmt::format("未找到头部对称骨骼 左侧 {}", "L_" + l_name).c_str()), MStatus::kFailure;
    if (!l_found_right)
      return MGlobal::displayError(fmt::format("未找到头部对称骨骼 右侧 {}", "R_" + l_name).c_str()), MStatus::kFailure;
  }

  return MStatus::kSuccess;
}

}  // namespace doodle::maya_plug