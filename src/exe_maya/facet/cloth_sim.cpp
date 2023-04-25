//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim.h"

#include "doodle_core/core/file_sys.h"
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/program_options.h>

#include "doodle_lib/exe_warp/maya_exe.h"

#include <maya_plug/main/maya_plug_fwd.h>

#include "cloth_sim/cloth_sim_factory.h"
#include "maya/MApiNamespace.h"
#include "maya/MStatus.h"
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MLibrary.h>

namespace doodle::maya_plug {
const std::string& cloth_sim::name() const noexcept {
  static const std::string name = "cloth_sim";
  return name;
}
bool cloth_sim::post() {
  auto l_str = FSys::from_quotation_marks(doodle_lib::Get().ctx().get<program_options>().arg(config).str());
  if (l_str.empty()) {
    return false;
  }
  is_init = true;
  DOODLE_LOG_INFO("开始初始化配置文件 {}", l_str);
  cloth_sim_factory l_factory{};

  try {
    auto l_json = nlohmann::json::parse(FSys::ifstream{l_str}).get<maya_exe_ns::qcloth_arg>();
    l_factory.preparation(l_json);
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return false;
  }

  MLibrary::initialize(true, "maya_doodle");
  MStatus l_status{};
  l_status = MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "ik2Bsolver";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "renderSetup";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "mayaHIK";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "OneClick";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "xgenToolkit";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "Unfold3D";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "MASH";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "mtoa";)");
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executeCommand(R"(loadPlugin "Substance";)");
  DOODLE_MAYA_CHICK(l_status);

  l_status = MFileIO::newFile(true);
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::executePythonCommand(R"(import pymel.core)");
  DOODLE_MAYA_CHICK(l_status);

  return true;
}

void cloth_sim::add_program_options() { doodle_lib::Get().ctx().get<program_options>().arg.add_param(config); }

cloth_sim::~cloth_sim() {
  if (is_init) MLibrary::cleanup(0, false);
}

};  // namespace doodle::maya_plug
