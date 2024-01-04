//
// Created by td_main on 2023/7/6.
//

#include "replace_file.h"
#ifdef fsin
#undef fsin
#endif
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/logger/logger.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <boost/asio.hpp>

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>

#include <exe_maya/core/maya_lib_guard.h>
namespace doodle::maya_plug {

bool replace_file_facet::post(const FSys::path& in_path) {
  bool l_ret = false;

  DOODLE_LOG_INFO("开始初始化配置文件 {}", in_path);
  maya_exe_ns::replace_file_arg l_arg{};
  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{in_path}).get<maya_exe_ns::replace_file_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return l_ret;
  }

  if (l_arg.file_path.empty()) return l_ret;

  lib_guard_ = std::make_shared<maya_lib_guard>(l_arg.maya_path);
  g_ctx().emplace<reference_file_factory>();
  l_ret = true;

  g_ctx().get<database_n::file_translator_ptr>()->async_open(l_arg.project_, false, true, g_reg(), [](auto&&) {});
  maya_file_io::set_workspace(l_arg.file_path);

  maya_file_io::open_file(l_arg.file_path);

  DOODLE_LOG_INFO("开始替换引用");

  auto l_s = boost::asio::make_strand(g_io_context());
  boost::asio::post(l_s, [this]() { this->create_ref_file(); });
  boost::asio::post(l_s, [l_files = l_arg.file_list, this]() { this->replace_file(l_files); });
  return l_ret;
}
void replace_file_facet::create_ref_file() {
  DOODLE_LOG_INFO("开始扫瞄引用");
  ref_files_ = g_ctx().get<reference_file_factory>().create_ref();
  ref_files_ |= ranges::actions::remove_if([](entt::handle& in_handle) -> bool {
    auto&& l_ref = in_handle.get<reference_file>();
    return l_ref.export_group_attr().has_value();
  });
}
void replace_file_facet::replace_file(const std::vector<std::pair<FSys::path, FSys::path>>& in_files) {
  for (auto&& l_pair : in_files) {
    auto l_ref_it = std::find_if(ref_files_.begin(), ref_files_.end(), [&l_pair](entt::handle& in_handle) -> bool {
      auto&& l_ref = in_handle.get<reference_file>();
      return l_ref.get_namespace() == l_pair.first.stem();
    });
    if (l_ref_it == ref_files_.end()) {
      DOODLE_LOG_ERROR("未查找到替换规则 {}", l_pair.first.string());
      continue;
    }
    auto&& l_ref = l_ref_it->get<reference_file>();
    l_ref.replace_file(l_pair.second);
  }

  DOODLE_LOG_INFO("替换完成");
  maya_file_io::save_file(
      maya_plug::maya_file_io::work_path("replace_file") / maya_plug::maya_file_io::get_current_path().filename()
  );
}

}  // namespace doodle::maya_plug