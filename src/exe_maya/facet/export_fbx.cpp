#include "export_fbx.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/database_task/sqlite_client.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/program_options.h>

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
struct maya_out_arg {
  FSys::path out_file{};
  // 引用文件
  FSys::path ref_file{};
  friend void to_json(nlohmann::json& j, const maya_out_arg& p) {
    j["out_file"] = p.out_file.generic_string();
    j["ref_file"] = p.ref_file.generic_string();
  }
};
void export_fbx_facet::create_ref_file() {
  DOODLE_LOG_INFO("开始扫瞄引用");
  ref_files_ = g_ctx().get<reference_file_factory>().create_ref(false);
}
void export_fbx_facet::export_fbx() {
  export_file_fbx l_ex{};
  std::vector<maya_out_arg> l_out_arg{};
  auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
  for (auto&& i : ref_files_) {
    if (i.get<reference_file>().export_group_attr()) {
      i.emplace<generate_file_path_ptr>(l_gen);
      auto l_path = l_ex.export_anim(i);
      if (!l_path.empty()) {
        l_out_arg.emplace_back(l_path, i.get<reference_file>().get_abs_path());
      }
    } else {
      auto l_path = i.get<reference_file>().get_abs_path();
      if (!l_path.empty()) l_out_arg.emplace_back(FSys::path{}, l_path);
    }
  }
  g_reg()->ctx().emplace<maya_camera>().conjecture();
  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace<generate_file_path_ptr>(l_gen);
  auto l_cam_path = l_ex.export_cam(l_h);

  l_out_arg.emplace_back(l_cam_path, FSys::path{});

  nlohmann::json l_json = l_out_arg;
  if (!out_path_file_.empty()) {
    if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
    FSys::ofstream{out_path_file_} << l_json.dump(4);
  } else
    log_info(fmt::format("导出文件 {}", l_json.dump(4)));
}

void export_fbx_facet::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p {};

  const MTime k_end_time = MAnimControl::maxTime();
  l_p.set_save_dir(maya_file_io::work_path() / "mov");
  l_p.conjecture_ep_sc();
  l_p.play_blast_(anim_begin_time_, k_end_time);
}

const std::string& export_fbx_facet::name() const noexcept {
  static const std::string name = "export_fbx_facet";
  return name;
}

bool export_fbx_facet::post() {
  bool l_ret = false;
  auto l_str = FSys::from_quotation_marks(g_ctx().get<program_options>().arg(config).str());
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
  out_path_file_ = l_arg.out_path_file_;

  lib_guard_     = std::make_shared<maya_lib_guard>(l_arg.maya_path);

  l_ret          = true;
  g_ctx().get<database_n::file_translator_ptr>()->set_only_open(true);
  g_ctx().get<database_n::file_translator_ptr>()->async_open(l_arg.project_);
  g_ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));

  maya_file_io::set_workspace(l_arg.file_path);

  maya_file_io::open_file(l_arg.file_path, l_arg.use_all_ref ? MFileIO::kLoadAllReferences : MFileIO::kLoadDefault);
  anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(l_arg.export_anim_time), MTime::uiUnit()};
  g_ctx().emplace<reference_file_factory>();
  DOODLE_LOG_INFO("开始导出fbx");
  auto l_s = boost::asio::make_strand(g_io_context());
  boost::asio::post(l_s, [this]() {
    this->create_ref_file();
    this->export_fbx();
  });
  if ((l_arg.bitset_ & maya_exe_ns::flags::k_create_play_blast).any()) {
    DOODLE_LOG_INFO("安排排屏");
    boost::asio::post(l_s, [l_s, this]() { this->play_blast(); });
  }
  DOODLE_LOG_INFO("复制文件maya文件");
  boost::asio::post(
      l_s,
      [l_source = l_arg.file_path,
       l_target =
           maya_plug::maya_file_io::work_path(FSys::path{"fbx"} / maya_plug::maya_file_io::get_current_path().stem()) /
           l_arg.file_path.filename()]() {
        FSys::copy_file(l_source, l_target, FSys::copy_options::overwrite_existing);
      }
  );

  return l_ret;
}

void export_fbx_facet::add_program_options() { g_ctx().get<program_options>().arg.add_param(config); }

}  // namespace doodle::maya_plug