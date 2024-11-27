//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/database_task/sqlite_client.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/app_base.h>
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

bool cloth_sim::post(const FSys::path& in_path) {
  bool l_ret = false;
  DOODLE_LOG_INFO("开始初始化配置文件 {}", in_path);
  maya_exe_ns::qcloth_arg l_arg{};

  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{in_path}).get<maya_exe_ns::qcloth_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return l_ret;
  }

  if (l_arg.file_path.empty()) return l_ret;

  lib_guard_     = std::make_shared<maya_lib_guard>(l_arg.maya_path);

  l_ret          = true;
  out_path_file_ = l_arg.out_path_file_;

  if (FSys::is_directory(l_arg.sim_path)) {
    for (auto&& l_p : FSys::directory_iterator(l_arg.sim_path)) {
      if (FSys::is_regular_file(l_p) && l_p.path().extension() == ".ma") {
        sim_file_map_[l_p.path().filename().string()] = l_p.path();
      }
    }
  }

  g_ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcExport";)"));
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "AbcImport";)"));
  (MGlobal::executeCommand(d_str{fmt::format(R"(loadPlugin "qualoth_{}_x64")", MAYA_APP_VERSION)}));

  maya_file_io::set_workspace(l_arg.file_path);
  maya_file_io::open_file(l_arg.file_path);
  maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));

  anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(l_arg.export_anim_time), MTime::uiUnit()};
  t_post_time_     = MTime{boost::numeric_cast<std::double_t>(l_arg.t_post), MTime::uiUnit()};
  DOODLE_LOG_INFO("tpost 开始时间 {}", t_post_time_);
  maya_chick(MAnimControl::setMinTime(t_post_time_));
  maya_chick(MAnimControl::setAnimationStartTime(t_post_time_));

  g_ctx().emplace<reference_file_factory>();

  maya_chick(MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(l_arg.t_post), MTime::uiUnit()}));

  auto l_s = boost::asio::make_strand(g_io_context());

  boost::asio::post(l_s, [l_s, this]() { this->create_ref_file(); });

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_replace_ref_file).any()) {
    DOODLE_LOG_INFO("安排替换引用");
    boost::asio::post(l_s, [l_s, this]() { this->replace_ref_file(); });
  }

  boost::asio::post(l_s, [l_s, this]() { this->create_cloth(); });

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_replace_ref_file).any()) {
    boost::asio::post(l_s, [l_s, this]() { this->set_cloth_attr(); });
  }

  if ((l_arg.bitset_ & maya_exe_ns::flags::k_sim_file).any()) {
    DOODLE_LOG_INFO("安排解算布料");
    boost::asio::post(l_s, [l_s, this]() { this->sim(); });
  } else if ((l_arg.bitset_ & maya_exe_ns::flags::k_touch_sim_file).any()) {
    DOODLE_LOG_INFO("安排touch布料");
    boost::asio::post(l_s, [l_s, this]() { this->touch_sim(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_create_play_blast).any()) {
    DOODLE_LOG_INFO("安排排屏");
    boost::asio::post(l_s, [l_s, this]() { this->play_blast(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_fbx_type).any()) {
    DOODLE_LOG_INFO("安排导出fbx");
    boost::asio::post(l_s, [l_s, this]() { this->export_fbx(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_abc_type).any()) {
    DOODLE_LOG_INFO("安排导出abc");
    boost::asio::post(l_s, [l_s, this]() { this->export_abc(); });
  }
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_export_anim_file).any()) {
    DOODLE_LOG_INFO("安排导出动画文件");
    boost::asio::post(l_s, [l_s, this]() { this->export_anim_file(); });
  }
  boost::asio::post(l_s, [l_s, this]() { this->write_config(); });
  boost::asio::post(l_s, [l_s, this]() { app_base::Get().stop_app(); });

  return l_ret;
}

};  // namespace doodle::maya_plug
