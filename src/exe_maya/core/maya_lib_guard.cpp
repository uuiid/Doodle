//
// Created by td_main on 2023/4/26.
//

#include "maya_lib_guard.h"

#include <doodle_core/doodle_core.h>
#include <doodle_core/logger/logger.h>

#include "maya_plug/data/maya_conv_str.h"
#include <maya_plug/logger/maya_logger_info.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MFragmentManager.h>
#include <maya/MGlobal.h>
#include <maya/MLibrary.h>
#include <memory>
// #include <boost/align.hpp>

#include <doodle_core/platform/win/register_file_type.h>

#include "../../../build/Ninja_debug/vcpkg_installed/x64-windows/include/boost/process/v2/environment.hpp"
#include <boost/process.hpp>

namespace doodle::maya_plug {

maya_lib_guard::maya_lib_guard() {
  MLibrary::initialize(true, "maya_doodle");
  doodle::g_logger_ctrl().add_log_sink(std::make_shared<::doodle::maya_plug::maya_msg_mt>(), "maya_plug");
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "ik2Bsolver";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "renderSetup";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "mayaHIK";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "OneClick";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "xgenToolkit";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "Unfold3D";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "MASH";)"));
  MGlobal::executeCommand(R"(loadPlugin "mtoa";)");
  MGlobal::executeCommand(R"(loadPlugin "Substance";)");
  default_logger_raw()->log(
      log_loc(), level::info, "env MAYA_MODULE_PATH {}",
      boost::this_process::environment()["MAYA_MODULE_PATH"].to_string()
  );
  maya_chick(MGlobal::executeCommand(conv::to_ms(fmt::format(R"(loadPlugin "doodle_maya_{}";)", MAYA_APP_VERSION))));

  if (MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer()) {
    if (auto* l_f = renderer->getFragmentManager()) {
      FSys::path l_path = boost::this_process::environment()["MAYA_LOCATION"].to_string();
      l_f->addFragmentPath(maya_plug::conv::to_ms((l_path / "bin" / "ScriptFragment").generic_string()));
      l_f->addFragmentPath(maya_plug::conv::to_ms((l_path / "bin" / "ShadeFragment").generic_string()));
    }
  }
  maya_chick(MGlobal::executePythonCommand(R"(import maya.cmds as cmds)"));
  MGlobal::executePythonCommand(R"(import pymel.core)");
  maya_chick(MFileIO::newFile(true));
}
maya_lib_guard::~maya_lib_guard() { MLibrary::cleanup(0, false); }
}  // namespace doodle::maya_plug