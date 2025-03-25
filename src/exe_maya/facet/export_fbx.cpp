#include "export_fbx.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include "doodle_lib/exe_warp/maya_exe.h"

#include "boost/system/detail/error_code.hpp"

#include "maya_plug/data/export_file_fbx.h"
#include "maya_plug/maya_plug_fwd.h"

#include <filesystem>
#include <fmt/format.h>
#include <memory>

#ifdef fsin
#undef fsin
#endif
#include <doodle_lib/long_task/image_to_move.h>

#include "boost/asio/post.hpp"
#include "boost/asio/strand.hpp"
#include "boost/numeric/conversion/cast.hpp"

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/maya_comm/file_info_edit.h>

#include "core/maya_lib_guard.h"
#include "maya/MApiNamespace.h"
#include "maya/MStatus.h"
#include "maya/MTime.h"
#include <cmath>
#include <exe_maya/data/play_blast.h>
#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

namespace doodle::maya_plug {

void export_fbx_facet::create_ref_file() {
  DOODLE_LOG_INFO("开始扫瞄引用");
  ref_files_ = g_ctx().get<reference_file_factory>().create_ref();
}
void export_fbx_facet::export_fbx() {
  export_file_fbx l_ex{};
  maya_exe_ns::maya_out_arg l_out_arg{};
  auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
  l_out_arg.begin_time  = anim_begin_time_.value();
  l_out_arg.end_time    = MAnimControl::maxTime().value();
  for (auto&& i : ref_files_) {
    if (i.export_group_attr()) {
      auto l_path = l_ex.export_anim(i, l_gen);
      if (!l_path.empty()) {
        l_out_arg.out_file_list.emplace_back(l_path, i.get_abs_path());
      }
    } else {
      auto l_path = i.get_abs_path();
      if (!l_path.empty()) l_out_arg.out_file_list.emplace_back(FSys::path{}, l_path);
    }
  }
  auto l_cam_path = l_ex.export_cam(l_gen, film_aperture_);

  l_out_arg.out_file_list.emplace_back(l_cam_path, FSys::path{});

  nlohmann::json l_json = l_out_arg;
  if (!out_path_file_.empty()) {
    if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
    default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
    FSys::ofstream{out_path_file_} << l_json.dump(4);
  } else
    log_info(fmt::format("导出文件 {}", l_json.dump(4)));
}

void export_fbx_facet::rig_file_export() {
  // export_file_fbx l_ex{};
  // maya_exe_ns::maya_out_arg l_out_arg{};
  // auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  // l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
  // l_out_arg.begin_time  = anim_begin_time_.value();
  // l_out_arg.end_time    = MAnimControl::maxTime().value();
  // auto l_out_path       = l_ex.export_rig();
  // l_out_arg.out_file_list.emplace_back(l_out_path, FSys::path{});
  // nlohmann::json l_json = l_out_arg;
  // if (!out_path_file_.empty()) {
  //   if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
  //   default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
  //   FSys::ofstream{out_path_file_} << l_json.dump(4);
  // } else
  //   log_info(fmt::format("导出文件 {}", l_json.dump(4)));
}

void export_fbx_facet::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p{};

  const MTime k_end_time = MAnimControl::maxTime();
  l_p.set_save_dir(maya_file_io::work_path() / "mov");
  l_p.conjecture_ep_sc();
  l_p.play_blast_(anim_begin_time_, k_end_time, size_);
}

bool export_fbx_facet::post(const nlohmann::json& in_argh) {
  bool l_ret                        = false;
  maya_exe_ns::export_fbx_arg l_arg = in_argh.get<maya_exe_ns::export_fbx_arg>();

  if (l_arg.file_path.empty()) return l_ret;
  out_path_file_ = l_arg.out_path_file_;
  film_aperture_ = l_arg.film_aperture_;
  size_          = l_arg.size_;

  l_ret          = true;
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));

  maya_file_io::set_workspace(l_arg.file_path);
  maya_file_io::open_file(l_arg.file_path, MFileIO::kLoadDefault);

  DOODLE_LOG_INFO("开始导出fbx");
  auto l_s = boost::asio::make_strand(g_io_context());
  // if (l_arg.rig_file_export) {
  //   DOODLE_LOG_INFO("导出rig文件");
  //   boost::asio::post(l_s, [this]() { this->rig_file_export(); });
  // } else {
  // }
  maya_chick(file_info_edit::delete_node_static());
  maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));
  anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
  g_ctx().emplace<reference_file_factory>();
  DOODLE_LOG_INFO("保存文件maya文件");
  boost::asio::post(
      l_s,
      [l_target =
           maya_plug::maya_file_io::work_path(FSys::path{"fbx"} / maya_plug::maya_file_io::get_current_path().stem()) /
           l_arg.file_path.filename()]() { maya_file_io::save_file(l_target); }
  );
  default_logger_raw()->info("导出动画中的文件");
  boost::asio::post(l_s, [this]() {
    this->create_ref_file();
    this->export_fbx();
  });
  if (l_arg.create_play_blast_) {
    DOODLE_LOG_INFO("安排排屏");
    boost::asio::post(l_s, [l_s, this]() { this->play_blast(); });
  }

  boost::asio::post(l_s, [](auto&&...) { app_base::Get().stop_app(); });

  return l_ret;
}

}  // namespace doodle::maya_plug