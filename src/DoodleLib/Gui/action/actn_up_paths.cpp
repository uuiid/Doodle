//
// Created by TD on 2021/6/21.
//

#include "actn_up_paths.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/AssetsPath.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/rpc/RpcFileSystemClient.h>

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
namespace doodle {

actn_up_paths::actn_up_paths()
    : p_tran() {
  p_name = "添加文件";
}
long_term_ptr actn_up_paths::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_term = this->get_long_term_signal();
  _arg_type   = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    k_term->sig_finished();
    k_term->sig_message_result("取消上传");
    return k_term;
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
  std::vector<AssetsPathPtr> k_ass_file_apth_list{};
  std::vector<AssetsPathPtr> k_ass_file_apth_list_filter{};

  /// 转换为资产路径
  std::transform(k_path.begin(), k_path.end(), std::back_inserter(k_ass_file_apth_list),
                 [k_ass_file](const FSys::path& in) {
                   return std::make_shared<AssetsPath>(in, k_ass_file);
                 });
  /// 转换为上传路径, 并在这时添加额外路径
  for (auto& k_i : k_ass_file_apth_list) {
    auto k_it = std::find_if(k_ass_file->getPathFile().begin(), k_ass_file->getPathFile().end(),
                             [k_i](const AssetsPathPtr& in_path) {
                               return in_path->getServerPath() == k_i->getServerPath();
                             });
    if (k_it != k_ass_file->getPathFile().end()) {
      if (Ue4Project::is_ue4_file(k_i->getLocalPath())) {
        if (FSys::exists(k_i->getLocalPath().parent_path() / Ue4Project::Content)) {
          auto k_ass_path = std::make_shared<AssetsPath>(
              k_i->getLocalPath().parent_path() / Ue4Project::Content, k_ass_file);
          k_list.emplace_back(std::make_unique<rpc_trans_path>(k_ass_path->getLocalPath(),
                                                               k_ass_path->getServerPath(),
                                                               k_ass_path->getBackupPath()));
        }
      }
      k_list.emplace_back(std::make_unique<rpc_trans_path>(k_i->getLocalPath(),
                                                           k_i->getServerPath(),
                                                           k_i->getBackupPath()));
    } else {
      if (Ue4Project::is_ue4_file(k_i->getLocalPath())) {
        if (FSys::exists(k_i->getLocalPath().parent_path() / Ue4Project::Content)) {
          auto k_ass_path = std::make_shared<AssetsPath>(
              k_i->getLocalPath().parent_path() / Ue4Project::Content, k_ass_file);
          k_list.emplace_back(std::make_unique<rpc_trans_path>(k_ass_path->getLocalPath(),
                                                               k_ass_path->getServerPath(),
                                                               k_ass_path->getBackupPath()));

          k_ass_file->getPathFile().push_back(k_ass_path);
        }
      }
      k_list.emplace_back(std::make_unique<rpc_trans_path>(k_i->getLocalPath(),
                                                           k_i->getServerPath(),
                                                           k_i->getBackupPath()));
      k_ass_file->getPathFile().push_back(k_i);
    }
  }
  k_ass_file->saved(true);
  p_tran = k_ch->Upload(k_list);
  k_term->forward_sig(p_tran->get_term());

  k_ass_file->updata_db();
  (*p_tran)();
  return k_term;
}
bool actn_up_paths::is_async() {
  return true;
}

actn_create_ass_up_paths::actn_create_ass_up_paths()
    : p_up(std::make_shared<actn_up_paths>()) {
  p_name = "创建并上传文件";
  p_up->sig_get_arg.connect([this]() { return _arg_type; });  /// 将信号和槽进行转移
}
bool actn_create_ass_up_paths::is_async() {
  return true;
}
long_term_ptr actn_create_ass_up_paths::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_term = this->get_long_term_signal();
  _arg_type   = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    this->cancel("取消上传");
    return k_term;
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
  return k_term;
}

}  // namespace doodle
