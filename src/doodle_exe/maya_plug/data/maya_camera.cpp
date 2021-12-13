//
// Created by TD on 2021/12/13.
//

#include "maya_camera.h"
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <doodle_lib/metadata/metadata.h>

#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/data/maya_file_io.h>

namespace doodle::maya_plug {

maya_camera::maya_camera() = default;

maya_camera::maya_camera(const MDagPath& in_path)
    : maya_camera() {
  p_path = in_path;
  chick();
}

void maya_camera::chick() const {
  MStatus k_s{};
  if (!p_path.isValid(&k_s)) {
    DOODLE_CHICK(k_s);
    throw doodle_error{"无效的dag 路径"};
  }
  if (!p_path.hasFn(MFn::Type::kCamera, &k_s)) {
    DOODLE_CHICK(k_s);
    throw doodle_error{"dag 路径不兼容 MFn::Type::kCamera"};
  }
}

bool maya_camera::export_file(const MTime& in_start, const MTime& in_end) {
  chick();

  MStatus k_s{};
  MSelectionList k_select{};
  k_s = k_select.add(p_path);
  DOODLE_CHICK(k_s);
  k_s = MGlobal::setActiveSelectionList(k_select);
  DOODLE_CHICK(k_s);
  /// \brief 开始创建路径并进行导出
  auto k_file_path = maya_file_io::work_path("fbx") / maya_file_io::get_current_path().stem();
  if (!FSys::exists(k_file_path))
    FSys::create_directories(k_file_path);
  k_file_path /= fmt::format("{}_camera_{}-{}.fbx",
                             maya_file_io::get_current_path().stem().generic_string(),
                             in_start.value(),
                             in_end.value());
  auto k_comm = fmt::format("FBXExportBakeComplexStart -v {};", in_start.value());
  k_s         = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format("FBXExportBakeComplexEnd -v {};", in_end.value());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = string{"FBXExportBakeComplexAnimation -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = string{"FBXExportConstraints -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format(R"(FBXExport -f "{}" -s;)", k_file_path.generic_string());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  return true;
}
bool maya_camera::back_camera(const MTime& in_start, const MTime& in_end) {
  chick();
  MStatus k_s{};
  auto k_comm = fmt::format(R"(bakeResults
-simulation true
-t "{}:{}"
-hierarchy below
-sampleBy 1
-oversamplingRate 1
-disableImplicitControl true
-preserveOutsideKeys true
-sparseAnimCurveBake false
-removeBakedAttributeFromLayer false
-removeBakedAnimFromLayer false
-bakeOnOverrideLayer false
-minimizeRotation true
-controlPoints false
-shape true
{{"{}"}};
)",
                            in_start.value(), in_end.value(), d_str{p_path.fullPathName(&k_s)}.str());
  DOODLE_CHICK(k_s);
  k_s = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);
  return true;
}
bool maya_camera::unlock_attr() {
  chick();
  MStatus k_s{};
  MFnDependencyNode k_node{};
  {
    auto k_obj = p_path.node(&k_s);
    DOODLE_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
  }
  const auto& k_size = k_node.attributeCount(&k_s);
  DOODLE_CHICK(k_s);
  for (int l_i = 0; l_i < k_size; ++l_i) {
    auto k_attr = k_node.attribute(l_i, &k_s);
    DOODLE_CHICK(k_s);
    auto k_plug = k_node.findPlug(k_attr, true, &k_s);
    if (k_plug.isLocked(&k_s)) {
      k_s = k_plug.setLocked(false);
      DOODLE_CHICK(k_s);
    }
  }

  return false;
}

}  // namespace doodle::maya_plug
