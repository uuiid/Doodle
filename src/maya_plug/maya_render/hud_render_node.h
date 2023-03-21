//
// Created by TD on 2021/11/15.
//

#pragma once

#include <maya/MPxDrawOverride.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MUserData.h>

namespace doodle::maya_plug {

class doodle_info_node : public MPxLocatorNode {
 public:
  doodle_info_node();
  ~doodle_info_node() override;

  static void* creator();
  static MStatus initialize();

  static MTypeId doodle_id;
  static MString drawDbClassification;
  static MString drawRegistrantId;
  const static constexpr auto node_type = MPxNode::Type::kLocatorNode;

  const static constexpr std::string_view node_name{"doolde_hud_render_node"};
};

class doodle_info_node_data : public MUserData {
 public:
  doodle_info_node_data();
  ~doodle_info_node_data() override;
};

class doodle_info_node_draw_override : public MPxDrawOverride {
  doodle_info_node_draw_override(const MObject& obj);

 public:
  static MPxDrawOverride* Creator(const MObject& obj);

  ~doodle_info_node_draw_override() override;

  MHWRender::DrawAPI supportedDrawAPIs() const override;

  bool isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const override;

  MBoundingBox boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const override;

  MUserData* prepareForDraw(
      const MDagPath& objPath, const MDagPath& cameraPath, const MFrameContext& frameContext, MUserData* oldData
  ) override;

  bool hasUIDrawables() const override { return true; }

  void addUIDrawables(
      const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext,
      const MUserData* data
  ) override;
};
}  // namespace doodle::maya_plug
