//
// Created by td_main on 2023/7/6.
//

#include "replace_file.h"
#ifdef fsin
#undef fsin
#endif
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <boost/asio.hpp>

#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_call_guard.h>
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/maya_comm/file_info_edit.h>

#include <exe_maya/core/maya_lib_guard.h>
#include <maya/MFileObject.h>
#include <maya/MFnReference.h>
#include <maya/MNamespace.h>
#include <maya/MSceneMessage.h>

namespace doodle::maya_plug {

bool replace_file_facet::post(const nlohmann::json& in_argh) {
  bool l_ret                          = false;

  maya_exe_ns::replace_file_arg l_arg = in_argh.get<maya_exe_ns::replace_file_arg>();

  if (l_arg.file_path.empty()) return l_ret;

  lib_guard_ = std::make_shared<maya_lib_guard>(l_arg.maya_path);
  g_ctx().emplace<reference_file_factory>();
  l_ret = true;

  maya_file_io::set_workspace(l_arg.file_path);

  auto l_s = boost::asio::make_strand(g_io_context());
  boost::asio::post(l_s, [l_files = l_arg.file_list, l_path = l_arg.file_path, this]() {
    this->replace_file(l_files, l_path);
  });
  boost::asio::post(l_s, [](auto&&...) { app_base::Get().stop_app(); });

  return l_ret;
}

void replace_file_facet::replace_file(
    const std::vector<std::pair<FSys::path, FSys::path>>& in_files, const FSys::path& in_file_path
) {
  struct tmp_data {
    std::vector<std::pair<FSys::path, FSys::path>> files_;
    FSys::path file_path_;
    std::vector<std::pair<MObject, std::string>> rename_namespaces{};
  };
  tmp_data l_data{in_files, in_file_path};

  MStatus k_s{};
  {
    maya_call_guard l_guard{MSceneMessage::addCheckReferenceCallback(
        MSceneMessage::kBeforeLoadReferenceCheck,
        [](bool* retCode, const MObject& referenceNode, MFileObject& file, void* clientData) {
          auto* self        = reinterpret_cast<tmp_data*>(clientData);
          FSys::path l_name = conv::to_s(file.rawName());
          auto l_it         = std::find_if(
              self->files_.begin(), self->files_.end(),
              [&l_name](const std::pair<FSys::path, FSys::path>& in_pair) -> bool {
                return l_name == in_pair.first.filename();
              }
          );
          if (l_it == self->files_.end()) {
            default_logger_raw()->log(log_loc(), level::info, "跳过引用文件 {}", l_name);
            *retCode = true;
            return;
          }

          MStatus k_s{};
          k_s = file.setRawFullName(conv::to_ms(l_it->second.generic_string()));
          DOODLE_MAYA_CHICK(k_s);
          *retCode = true;

          self->rename_namespaces.emplace_back(referenceNode, l_it->second.stem().generic_string());
          default_logger_raw()->log(log_loc(), level::info, "替换加载引用文件 {}", l_it->second);
        },
        &l_data
    )};
    DOODLE_LOG_INFO("开始替换引用");
    maya_file_io::open_file(in_file_path);
  }
  DOODLE_LOG_INFO("开始扫瞄引用");
  ref_files_ = g_ctx().get<reference_file_factory>().create_ref();

  // rename namespace
  DOODLE_LOG_INFO("开始重命名命名空间");
  MFnReference l_fn_ref{};
  for (auto&& l_pair : l_data.rename_namespaces) {
    k_s = l_fn_ref.setObject(l_pair.first);
    DOODLE_MAYA_CHICK(k_s);
    auto l_namespace = l_fn_ref.associatedNamespace(true);
    if (l_namespace.length() == 0) continue;
    k_s = MNamespace::renameNamespace(l_namespace, conv::to_ms(l_pair.second));
    DOODLE_MAYA_CHICK(k_s);
  }

  for (auto&& l_pair : l_data.rename_namespaces) {
    file_info_edit::refresh_node(l_pair.first);
  }

  DOODLE_LOG_INFO("重命名完成");
  maya_file_io::save_file(
      maya_plug::maya_file_io::work_path("replace_file") / maya_plug::maya_file_io::get_current_path().filename()
  );
}

}  // namespace doodle::maya_plug