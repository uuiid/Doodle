#include "export_fbx.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/database_task/sqlite_client.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/program_options.h>

#include "doodle_lib/exe_warp/maya_exe.h"

#include <fmt/format.h>

#ifdef fsin
#undef fsin
#endif
#include <doodle_lib/long_task/image_to_move.h>

#include "boost/asio/post.hpp"
#include "boost/asio/strand.hpp"
#include "boost/numeric/conversion/cast.hpp"

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/reference_file.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "core/maya_lib_guard.h"
#include "maya/MApiNamespace.h"
#include "maya/MStatus.h"
#include "maya/MTime.h"
#include <cmath>
#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
namespace doodle::maya_plug {
const std::string& export_fbx_facet::name() const noexcept {
  static const std::string name = "export_fbx_facet";
  return name;
}
bool export_fbx_facet::post() {
  bool l_ret = false;
  auto l_str = FSys::from_quotation_marks(doodle_lib::Get().ctx().get<program_options>().arg(config).str());
  if (l_str.empty()) {
    return l_ret;
  }
  DOODLE_LOG_INFO("开始初始化配置文件 {}", l_str);
  maya_exe_ns::export_fbx_arg l_arg{};

  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{l_str}).get<maya_exe_ns::export_fbx_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return l_ret;
  }

  if (l_arg.file_path.empty()) return l_ret;

  lib_guard_ = std::make_shared<maya_lib_guard>();

  l_ret      = true;

  doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->open_(l_arg.project_);
  doodle_lib::Get().ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcExport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcImport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));

  maya_file_io::set_workspace(l_arg.file_path);

  maya_file_io::open_file(l_arg.file_path, l_arg.use_all_ref ? MFileIO::kLoadAllReferences : MFileIO::kLoadDefault);
  anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(l_arg.export_anim_time), MTime::uiUnit()};
  DOODLE_LOG_INFO("开始导出fbx");
  return l_ret;
}

void export_fbx_facet::add_program_options() { doodle_lib::Get().ctx().get<program_options>().arg.add_param(config); }
}  // namespace doodle::maya_plug