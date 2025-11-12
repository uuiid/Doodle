//
// Created by TD on 25-3-24.
//

#include "export_rig_facet.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/app_base.h>

#include "doodle_lib/exe_warp/maya_exe.h"

#include "maya_plug/data/export_file_fbx.h"

#include "export_fbx.h"
#include <filesystem>
#include <fmt/format.h>
#include <memory>

#ifdef fsin
#undef fsin
#endif
#include <doodle_lib/long_task/image_to_move.h>

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/maya_comm/file_info_edit.h>

#include <cmath>
#include <exe_maya/data/play_blast.h>
#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

namespace doodle::maya_plug {

bool export_rig_facet::post(const nlohmann::json& in_argh) {
  bool l_ret                        = false;
  maya_exe_ns::export_rig_arg l_arg = in_argh.get<maya_exe_ns::export_rig_arg>();

  if (l_arg.get_file_path().empty()) return l_ret;
  out_path_file_ = l_arg.get_out_path_file();

  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));

  maya_file_io::set_workspace(l_arg.get_file_path());
  maya_file_io::open_file(l_arg.get_file_path(), MFileIO::kLoadDefault);

  DOODLE_LOG_INFO("开始导出 rig fbx");
  auto l_s = boost::asio::make_strand(g_io_context());
  maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));
  anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};

  export_file_fbx l_ex{};
  maya_exe_ns::maya_out_arg l_out_arg{};
  auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
  l_out_arg.begin_time  = anim_begin_time_.value();
  l_out_arg.end_time    = MAnimControl::maxTime().value();
  reference_file l_ref{};
  cloth_factory_interface l_cf{};
  std::vector<cloth_interface> l_cloth_interfaces{};
  if (qcloth_factory::has_cloth()) {
    l_cf               = std::make_shared<qcloth_factory>();
    l_cloth_interfaces = l_cf->create_cloth();
  }
  auto l_out_path = l_ex.export_rig(l_ref);
  for (auto&& p : l_out_path) l_out_arg.out_file_list.emplace_back(p, FSys::path{});
  nlohmann::json l_json = l_out_arg;
  if (!out_path_file_.empty()) {
    if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
    default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
    FSys::ofstream{out_path_file_} << l_json.dump(4);
  } else
    log_info(fmt::format("导出文件 {}", l_json.dump(4)));
  boost::asio::post(l_s, [](auto&&...) { app_base::Get().stop_app(); });
  return l_ret;
}
}  // namespace doodle::maya_plug