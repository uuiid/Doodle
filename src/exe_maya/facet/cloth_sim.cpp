//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/database_task/sqlite_client.h"
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/program_options.h>

#include "doodle_lib/exe_warp/maya_exe.h"

#include "boost/asio/post.hpp"
#include "boost/asio/strand.hpp"
#include "boost/numeric/conversion/cast.hpp"

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/main/maya_plug_fwd.h>

#include "core/maya_lib_guard.h"
#include "maya/MApiNamespace.h"
#include "maya/MStatus.h"
#include <cmath>
#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

namespace doodle::maya_plug {
const std::string& cloth_sim::name() const noexcept {
  static const std::string name = "cloth_sim";
  return name;
}
bool cloth_sim::post() {
  bool l_ret = false;
  auto l_str = FSys::from_quotation_marks(doodle_lib::Get().ctx().get<program_options>().arg(config).str());
  if (l_str.empty()) {
    return l_ret;
  }
  DOODLE_LOG_INFO("开始初始化配置文件 {}", l_str);
  maya_exe_ns::qcloth_arg l_arg{};

  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{l_str}).get<maya_exe_ns::qcloth_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return l_ret;
  }

  if (l_arg.file_path.empty()) return l_ret;

  anim_begin_time_ = l_arg.export_anim_time;
  lib_guard_       = std::make_shared<maya_lib_guard>();

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

  maya_chick(MFileIO::newFile(true));
  maya_chick(MGlobal::executePythonCommand(R"(import maya.cmds as cmds)"));
  maya_chick(MGlobal::executePythonCommand(R"(import pymel.core)"));

  doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->open_(l_arg.project_);

  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcExport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcImport";)"));
  MGlobal::executeCommand(d_str{fmt::format(R"(loadPlugin "qualoth_{}_x64")", MAYA_APP_VERSION)});

  maya_file_io::set_workspace(l_arg.file_path);

  maya_file_io::open_file(l_arg.file_path);

  maya_chick(MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(l_arg.t_post), MTime::uiUnit()}));

  auto l_s = boost::asio::make_strand(g_io_context());

  boost::asio::post(l_s, [l_s, this]() { this->create_ref_file(); });

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_replace_ref_file).any()) {
    DOODLE_LOG_INFO("开始替换引用");
    boost::asio::post(l_s, [l_s, this]() { this->replace_ref_file(); });
  }

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_sim_file).any()) {
    DOODLE_LOG_INFO("开始解算布料");
    boost::asio::post(l_s, [l_s, this]() {
      this->create_cloth();
      this->sim();
    });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_create_play_blast).any()) {
    DOODLE_LOG_INFO("开始排屏");
    boost::asio::post(l_s, [l_s, this]() { this->play_blast(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_fbx_type).any()) {
    DOODLE_LOG_INFO("开始导出fbx");
    boost::asio::post(l_s, [l_s, this]() { this->export_abc(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_abc_type).any()) {
    DOODLE_LOG_INFO("开始导出abc");
    boost::asio::post(l_s, [l_s, this]() { this->export_fbx(); });
  }

  return l_ret;
}

void cloth_sim::add_program_options() { doodle_lib::Get().ctx().get<program_options>().arg.add_param(config); }

};  // namespace doodle::maya_plug
