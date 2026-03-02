#include "dna_calib_node.h"

#include "maya_plug_fwd.h"
#include <maya_plug/node/dna_calib_node_impl.h>

#include <data/maya_tool.h>
#include <fmt/xchar.h>
#include <maya/MAngle.h>
#include <maya/MApiNamespace.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <ranges>

namespace doodle::maya_plug {
MObject dna_calib_node::dna_file_path{};
MObject dna_calib_node::gui_control_list{};
MObject dna_calib_node::output_join_transforms{};
MObject dna_calib_node::output_join_rotations{};
MObject dna_calib_node::output_join_scales{};
MObject dna_calib_node::output_blendshape_weights{};

MTypeId dna_calib_node::doodle_id = MTypeId{0x00000002};

dna_calib_node::dna_calib_node() : p_i(std::make_unique<impl_t>()) { p_i->self_ = this; }
dna_calib_node::~dna_calib_node() = default;

void* dna_calib_node::creator() { return new dna_calib_node{}; }
dna_calib_node::impl_t* dna_calib_node::impl() { return p_i.get(); }
MStatus dna_calib_node::initialize() {
  MStatus l_status{};

  {  // DNA文件路径
    MFnTypedAttribute l_typed_attr{};
    dna_file_path = l_typed_attr.create("dna_file_path", "dna_p", MFnData::kString, MObject::kNullObj, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(dna_file_path));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    gui_control_list = l_numeric_attr.create("gui_control_list", "gui_cl", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(gui_control_list));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_join_transforms =
        l_numeric_attr.create("output_join_transforms", "out_jt", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_transforms));
  }
  {
    MFnUnitAttribute l_numeric_attr{};
    output_join_rotations =
        l_numeric_attr.create("output_join_rotations", "out_jr", MFnUnitAttribute::kAngle, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_rotations));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_join_scales = l_numeric_attr.create("output_join_scales", "out_js", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_scales));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_blendshape_weights =
        l_numeric_attr.create("output_blendshape_weights", "out_bsw", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_blendshape_weights));
  }
#define DOODLE_ATTRAFF(attr)                                                          \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_transforms)); \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_rotations));  \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_scales));     \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_blendshape_weights));
  DOODLE_ATTRAFF(gui_control_list);
#undef DOODLE_ATTRAFF
  return l_status;
}
MStatus dna_calib_node::compute(const MPlug& in_plug, MDataBlock& in_data_block) {
  if (in_plug == output_join_transforms || in_plug == output_join_rotations || in_plug == output_join_scales ||
      in_plug == output_blendshape_weights) {
    if (!impl()->rig_instance_ptr_) return display_error("请先创建rig数据"), MS::kFailure;
    MStatus l_status{};
    // 确保 gui_control_list 的数据被正确读取到内存中, 以便在 rig 计算中使用
    auto l_gui_control = in_data_block.inputArrayValue(gui_control_list, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_gui_control_count = impl()->dna_calib_dna_reader_->getGUIControlCount();
    for (auto i = 0; i < l_gui_control.elementCount(); ++i, l_gui_control.next()) {
      auto l_gui_control_plug = l_gui_control.inputValue(&l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      auto l_gui_control_value = l_gui_control_plug.asDouble();
      auto l_index             = l_gui_control.elementIndex(&l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      if (l_index >= l_gui_control_count) {
        display_warning(
            "gui_control_list 中的元素数量 {} 超过了 dna 文件中 GUI Control 的数量 {}, 跳过剩余元素", l_index + 1,
            l_gui_control_count
        );
        break;
      }
      impl()->rig_instance_ptr_->setGUIControl(l_index, l_gui_control_value);
    }
    // {
    //   auto l_gui_control_info = impl()->rig_instance_ptr_->getGUIControlValues();
    //   for (auto i = 0; i < l_gui_control_info.size(); i += 100)
    //     display_info(
    //         "GUI Control Values: {}", fmt::join(l_gui_control_info | std::views::drop(i) | std::views::take(100), ",
    //         ")
    //     );
    // }

    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(impl()->compute());

    auto l_out_j_t = in_data_block.outputArrayValue(output_join_transforms, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_j_r = in_data_block.outputArrayValue(output_join_rotations, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_j_s = in_data_block.outputArrayValue(output_join_scales, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_bsw = in_data_block.outputArrayValue(output_blendshape_weights, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_j_t_bl = l_out_j_t.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_j_r_bl = l_out_j_r.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_j_s_bl = l_out_j_s.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_out_bsw_bl = l_out_bsw.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    // 清空原有数据
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t_bl.growArray(0));
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r_bl.growArray(0));
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s_bl.growArray(0));
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_bsw_bl.growArray(0));

    // 关节输出数据 [tx, ty, tz, rx, ry, rz, sx, sy, sz] * joint_count
    // todo: 查找文档后, 发现每个关节不是 9 个属性都会计算, 需要对属性进行筛选, 目前先假设每个关节都有完整的 9
    // 个输出属性
    auto l_raw_j      = impl()->rig_instance_ptr_->getJointOutputs();
    auto l_raw_bsw    = impl()->rig_instance_ptr_->getBlendShapeOutputs();
    auto& l_joint_map = impl()->joint_decode_cache(impl()->rig_instance_ptr_->getLOD());

    if (std::ranges::all_of(l_raw_j, [](auto& v) { return v == 0; }))
      display_error("RigLogic 输出的关节属性全为 0 计算失败");

    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t_bl.growArray(static_cast<unsigned int>(l_raw_j.size())));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r_bl.growArray(static_cast<unsigned int>(l_raw_j.size())));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s_bl.growArray(static_cast<unsigned int>(l_raw_j.size())));
    if (l_joint_map.size() != l_raw_j.size())
      return display_error(
                 "Joint Decode Cache 的大小 {} 与 RigInstance 输出的关节属性数量 {} 不匹配", l_joint_map.size(),
                 l_raw_j.size()
             ),
             MS::kFailure;

    const auto& l_trans_scale = impl()->trans_scale_;

    const auto& l_to_mangle   = impl()->to_mangle_func_;

    for (auto i = 0; i < l_raw_j.size(); ++i) {
      switch (l_joint_map[i].attribute_index_) {
        case 0:
        case 1:
        case 2:
          l_out_j_t_bl.addElement(i, &l_status).set((l_raw_j[i] + l_joint_map[i].neutral_value_) * l_trans_scale);
          DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
          break;  // tx, ty, tz
        case 3:
        case 4:
        case 5: {
          auto l_h = l_out_j_r_bl.addElement(i, &l_status);
          DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
          l_h.setMAngle(l_to_mangle(l_raw_j[i] + l_joint_map[i].neutral_value_));
        } break;  // rx, ry, rz
        case 6:
        case 7:
        case 8:
          l_out_j_s_bl.addElement(i, &l_status).set(l_raw_j[i] + l_joint_map[i].neutral_value_);
          DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
          break;  // sx, sy, sz
      }
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t.set(l_out_j_t_bl));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r.set(l_out_j_r_bl));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s.set(l_out_j_s_bl));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t.setAllClean());
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r.setAllClean());
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s.setAllClean());
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_bsw.setAllClean());
  }
  return MS::kUnknownParameter;
}

}  // namespace doodle::maya_plug