//
// Created by TD on 2021/7/16.
//

#include "actn_maya_export.h"

#include <DoodleLib/FileWarp/MayaFile.h>
#include <Gui/action/actn_up_paths.h>
#include <Metadata/Metadata_cpp.h>
#include <core/CoreSet.h>
#include <core/DoodleLib.h>
#include <rpc/RpcFileSystemClient.h>
#include <threadPool/ThreadPool.h>
namespace doodle {

actn_maya_export::actn_maya_export()
    : p_up_paths(std::make_shared<actn_create_ass_up_paths>()) {
  p_name = "导出maya fbx 并上传文件";
  p_term = std::make_shared<long_term>();
  p_up_paths->sig_get_arg.connect([this]() {
    actn_up_paths::arg_ k_arg{};
    k_arg.date.push_back(p_paths);
    return k_arg;
  });
}
bool actn_maya_export::is_async() {
  return true;
}
long_term_ptr actn_maya_export::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    p_term->sig_finished();
    p_term->sig_message_result("已取消");
    return p_term;
  }
  auto k_path = std::dynamic_pointer_cast<AssetsFile>(in_data)->getPathFile().front();
  auto k_list = std::make_unique<rpc_trans_path>(
      k_path->get_cache_path(),
      k_path->getServerPath());
  DoodleLib::Get().getRpcFileSystemClient()->Download(k_list)->wait();
  _arg_type.date = k_path->get_cache_path();

  auto k_maya = std::make_shared<MayaFile>();
  k_maya->get_term()->forward_sig(p_term);
  p_term->sig_finished.connect([this, in_data, in_parent]() {
    p_up_paths->run(in_data, in_parent);
  });

  p_paths = CoreSet::getSet().getCacheRoot("maya_export") / _arg_type.date.stem();
  DoodleLib::Get().get_thread_pool()->enqueue([k_maya, this]() {
    k_maya->exportFbxFile(_arg_type.date, p_paths);
  });

  return p_term;
}
bool actn_maya_export::is_accept(const action_arg::arg_path& in_any) {
  return MayaFile::is_maya_file(in_any.date);
}
actn_maya_export_batch::actn_maya_export_batch()
    : p_up_paths(),
      p_paths() {
}
long_term_ptr actn_maya_export_batch::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  return doodle::long_term_ptr();
}
bool actn_maya_export_batch::is_async() {
  return false;
}

}  // namespace doodle
