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

#include "maya/MApiNamespace.h"
#include "maya/MStatus.h"
#include <cmath>
#include <maya/MAnimControl.h>
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
  maya_exe_ns::qcloth_arg l_arg{};

  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{l_str}).get<maya_exe_ns::qcloth_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return false;
  }

  if (l_arg.file_path.empty()) return false;

  anim_begin_time_ = l_arg.export_anim_time;
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

  doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->open_(l_arg.project_);

  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcExport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcImport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));
  MGlobal::executeCommand(d_str{fmt::format(R"(loadPlugin "qualoth_{}_x64")", MAYA_APP_VERSION)});

  maya_file_io::set_workspace(l_arg.file_path);

  maya_file_io::open_file(l_arg.file_path);

  MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(l_arg.t_post), MTime::uiUnit()});

  auto l_s = boost::asio::make_strand(g_io_context());

  boost::asio::post(l_s, [l_s, this]() { this->create_ref_file(); });

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_replace_ref_file).any()) {
    boost::asio::post(l_s, [l_s, this]() { this->replace_ref_file(); });
  }

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_sim_file).any()) {
    boost::asio::post(l_s, [l_s, this]() {
      this->create_cloth();
      this->sim();
    });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_create_play_blast).any()) {
    boost::asio::post(l_s, [l_s, this]() { this->play_blast(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_fbx_type).any()) {
    boost::asio::post(l_s, [l_s, this]() { this->export_abc(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_abc_type).any()) {
    boost::asio::post(l_s, [l_s, this]() { this->export_fbx(); });
  }

  return true;
}

void cloth_sim::add_program_options() { doodle_lib::Get().ctx().get<program_options>().arg.add_param(config); }

cloth_sim::~cloth_sim() {
  if (is_init) MLibrary::cleanup(0, false);
}

};  // namespace doodle::maya_plug
