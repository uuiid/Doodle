//
// Created by TD on 2024/1/11.
//

#include "files_info.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MTypeId.h>
namespace doodle::maya_plug {
MTypeId doodle_file_info::doodle_id = MTypeId{0x00000001};

MObject doodle_file_info::reference_file;
MObject doodle_file_info::reference_file_path;
MObject doodle_file_info::reference_file_namespace;
MObject doodle_file_info::is_solve;
MObject doodle_file_info::collision_objects;
MObject doodle_file_info::wind_field;

void *doodle_file_info::creator() { return new doodle_file_info{}; }
MStatus doodle_file_info::initialize() {
  MStatus l_status{};

  {
    MFnMessageAttribute l_msg_attr{};
    reference_file = l_msg_attr.create("reference_file", "ref", &l_status);
    maya_chick(l_status);
    l_msg_attr.setStorable(true);
    l_msg_attr.setWritable(true);
    l_msg_attr.setConnectable(true);
    addAttribute(reference_file);
  }

  {  // 引用文件路径, MString 类型
    MFnTypedAttribute l_typed_attr{};
    reference_file_path =
        l_typed_attr.create("reference_file_path", "ref_p", MFnData::kString, MObject::kNullObj, &l_status);
    maya_chick(l_status);
    l_typed_attr.setStorable(true);
    l_typed_attr.setWritable(true);

    addAttribute(reference_file_path);
  }

  {  // 引用文件所在的名称空间 MString 类型
    MFnTypedAttribute l_typed_attr{};
    reference_file_namespace =
        l_typed_attr.create("reference_file_namespace", "ref_np", MFnData::kString, MObject::kNullObj, &l_status);
    maya_chick(l_status);
    l_typed_attr.setStorable(true);
    l_typed_attr.setWritable(true);

    addAttribute(reference_file_namespace);
  }

  {  // 是否解算
    MFnNumericAttribute l_numeric_attr{};
    is_solve = l_numeric_attr.create("is_solve", "is_solve", MFnNumericData::kBoolean, false, &l_status);
    maya_chick(l_status);
    l_numeric_attr.setStorable(true);
    l_numeric_attr.setWritable(true);

    addAttribute(is_solve);
  }
  {
    MFnMessageAttribute l_msg_attr{};
    collision_objects = l_msg_attr.create("collision_objects", "collision_objects", &l_status);
    maya_chick(l_status);
    l_msg_attr.setStorable(true);
    l_msg_attr.setArray(true);
    l_msg_attr.setWritable(true);
    l_msg_attr.setConnectable(true);
    l_msg_attr.setDisconnectBehavior(MFnAttribute::DisconnectBehavior::kDelete);

    addAttribute(collision_objects);
  }
  {
    MFnMessageAttribute l_msg_attr{};
    wind_field = l_msg_attr.create("wind_field", "wind_field", &l_status);
    maya_chick(l_status);
    l_msg_attr.setStorable(true);
    l_msg_attr.setWritable(true);
    l_msg_attr.setConnectable(true);
    l_msg_attr.setDisconnectBehavior(MFnAttribute::DisconnectBehavior::kDelete);
    addAttribute(wind_field);
  }

  return MStatus::kSuccess;
}
MStatus doodle_file_info::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc) {
  if (plug == reference_file) {
    MFnDependencyNode l_fn_node{thisMObject()};
    MPlug l_plug = l_fn_node.findPlug(reference_file_path, true);
    l_plug.setValue(MString{});
    l_plug = l_fn_node.findPlug(reference_file_namespace, true);
    l_plug.setValue(MString{});
    l_plug = l_fn_node.findPlug(is_solve, true);
    l_plug.setValue(false);
  }
  return MPxNode::connectionBroken(plug, otherPlug, asSrc);
}

}  // namespace doodle::maya_plug