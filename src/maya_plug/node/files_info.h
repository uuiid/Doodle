//
// Created by TD on 2024/1/11.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
namespace doodle::maya_plug {
class doodle_file_info : public MPxNode {
  friend class reference_file;
  friend class file_info_edit;
  friend class reference_attr_setting;

 public:
  static MTypeId doodle_id;
  const static constexpr auto node_type = MPxNode::Type::kDependNode;
  static void* creator();
  static MStatus initialize();

  MStatus connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;

  const static constexpr std::string_view node_name{"doodle_file_info"};

 private:
  // 引用文件
  static MObject reference_file;
  // 引用文件路径
  static MObject reference_file_path;
  // 引用文件命名空间
  static MObject reference_file_namespace;

  // 是否解算
  static MObject is_solve;
  // 碰撞物体
  static MObject collision_objects;
  // 风场
  static MObject wind_field;

  static MObject simple_subsampling;
  static MObject frame_samples;
  static MObject time_scale;
  static MObject length_scale;
  static MObject max_cg_iteration;
  static MObject cg_accuracy;
  static MObject gravity;
};

}  // namespace doodle::maya_plug