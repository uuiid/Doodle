#include <MayaPlug/MotionMayaPlugInit.h>

#include <Maya/MApiNamespace.h>
#include <Maya/MObject.h>
#include <Maya/MFnPlugin.h>

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj, "1.0", "Any"};

  status = k_plugin.registerCommand("doodletest", doodletest::create);
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj};
  status = k_plugin.deregisterCommand("doodletest");
  return status;
}