//
// Created by TD on 2024/1/11.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
namespace doodle::maya_plug {
class doodle_file_info : public MPxNode {
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
  static MObject wind_fields;
};

}  // namespace doodle::maya_plug