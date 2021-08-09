//
// Created by TD on 2021/6/21.
//

#include "actn_up_paths.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/AssetsPath.h>
#include <Metadata/MetadataFactory.h>
#include <core/DoodleLib.h>
#include <rpc/RpcFileSystemClient.h>

namespace doodle {

actn_up_paths::actn_up_paths()
    : p_tran() {
  p_name = "添加文件";
  p_term = std::make_shared<long_term>();
}
long_term_ptr actn_up_paths::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    p_term->sig_finished();
    p_term->sig_message_result("取消上传");
    return p_term;
  }

  auto k_ch = DoodleLib::Get().getRpcFileSystemClient();

  auto k_path = _arg_type.date;
  AssetsFilePtr k_ass_file;

  k_ass_file = std::dynamic_pointer_cast<AssetsFile>(in_data);
  if (!k_ass_file) {
    DOODLE_LOG_DEBUG("无效的上传数据")
    throw DoodleError{"无效的上传数据"};
  }

  rpc_trans_path_ptr_list k_list{};
  for (auto& k_i : k_path) {
    auto k_ass_path = std::make_shared<AssetsPath>(k_i, k_ass_file);
    k_list.emplace_back(std::make_unique<rpc_trans_path>(k_ass_path->getLocalPath(),
                                                         k_ass_path->getServerPath(),
                                                         k_ass_path->getBackupPath()));
    k_ass_file->getPathFile().push_back(k_ass_path);
  }

  p_tran = k_ch->Upload(k_list);
  p_term->forward_sig(p_tran->get_term());

  k_ass_file->updata_db();
  (*p_tran)();
  return p_term;
}
bool actn_up_paths::is_async() {
  return true;
}

actn_create_ass_up_paths::actn_create_ass_up_paths()
    : p_up(std::make_shared<actn_up_paths>()) {
  p_name = "创建并上传文件";
  p_term = p_up->get_long_term_signal();
  p_up->sig_get_arg.connect([this]() { return _arg_type; });  /// 将信号和槽进行转移
}
bool actn_create_ass_up_paths::is_async() {
  return true;
}
long_term_ptr actn_create_ass_up_paths::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    p_term->sig_finished();
    p_term->sig_message_result("取消上传");
    return p_term;
  }
  AssetsFilePtr k_ass_file;
  if (in_parent) {
    auto k_str = in_parent->showStr();
    k_ass_file = std::make_shared<AssetsFile>(in_parent, k_str);
    in_parent->child_item.push_back_sig(k_ass_file);

    k_ass_file->setVersion(k_ass_file->find_max_version());
  }
  if (!k_ass_file) {
    DOODLE_LOG_DEBUG("无效的上传数据")
    throw DoodleError{"无效的上传数据"};
  }
  (*p_up)(k_ass_file, in_parent);
  return p_term;
}

}  // namespace doodle
