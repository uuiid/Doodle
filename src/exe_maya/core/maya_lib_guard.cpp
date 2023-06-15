//
// Created by td_main on 2023/4/26.
//

#include "maya_lib_guard.h"

#include <doodle_core/doodle_core.h>
#include <doodle_core/logger/logger.h>

#include <maya_plug/logger/maya_logger_info.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MFileIO.h>
#include <maya/MLibrary.h>
#include <memory>
// #include <boost/align.hpp>
namespace doodle {
namespace maya_plug {
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
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "mtoa";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "Substance";)"));

  maya_chick(MGlobal::executePythonCommand(R"(import maya.cmds as cmds)"));
  maya_chick(MGlobal::executePythonCommand(R"(import pymel.core)"));
  maya_chick(MFileIO::newFile(true));
}
maya_lib_guard::~maya_lib_guard() { MLibrary::cleanup(0, false); }
}  // namespace maya_plug
}  // namespace doodle