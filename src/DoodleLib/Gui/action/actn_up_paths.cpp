//
// Created by TD on 2021/6/21.
//

#include "actn_up_paths.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/AssetsPath.h>
#include <rpc/RpcFileSystemClient.h>

namespace doodle {

actn_up_paths::actn_up_paths()
    : p_tran() {
  p_name = "直接上传多个路径";
  p_term = std::make_shared<long_term>();
}
long_term_ptr actn_up_paths::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_ch = CoreSet::getSet().getRpcFileSystemClient();

  auto k_path = sig_get_arg().value().date;
  AssetsFilePtr k_ass_file;

  //  if (in_data)
  //    k_ass_file = std::dynamic_pointer_cast<AssetsFile>(in_data);
  //  else
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

  rpc_trans_path_ptr_list k_list{};
  std::vector<AssetsPathPtr> k_ass_path_list;
  for (auto& k_i : k_path) {
    auto k_ass_path = k_ass_path_list.emplace_back(std::make_shared<AssetsPath>(k_i, k_ass_file));
    k_list.emplace_back(std::make_unique<rpc_trans_path>(k_ass_path->getLocalPath(),
                                                         k_ass_path->getServerPath(),
                                                         k_ass_path->getBackupPath()));
  }
  p_tran = k_ch->Upload(k_list);
  p_tran->get_term()->forward_sig(p_term);


  k_ass_file->setPathFile(k_ass_path_list);
  k_ass_file->updata_db(in_parent->getMetadataFactory());
  (*p_tran)();
  return p_term;
}
bool actn_up_paths::is_async() {
  return true;
}
}  // namespace doodle
