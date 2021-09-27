//
// Created by TD on 2021/5/18.
//

#include "assets_path.h"

#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/Metadata/assets_file.h>
#include <DoodleLib/Metadata/metadata.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/core/doodle_lib.h>
#include <Logger/logger.h>
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path)
namespace doodle {
assets_path::assets_path()
    : p_local_path(),
      p_lexically_relative(),
      p_server_path(),
      p_backup_path("backup/") {
}
assets_path::assets_path(const FSys::path &in_path, const MetadataConstPtr &in_metadata)
    : p_local_path(),
      p_lexically_relative(),
      p_server_path(),
      p_backup_path("backup/") {
  if (in_metadata)
    this->setPath(in_path, in_metadata);
  else
    throw doodle_error{"空指针"};
}
const FSys::path &assets_path::getLocalPath() const {
  return p_local_path;
}

const FSys::path &assets_path::getServerPath() const {
  return p_server_path;
}

const FSys::path &assets_path::getBackupPath() const {
  return p_backup_path;
}

void assets_path::setPath(const FSys::path &in_path, const MetadataConstPtr &in_metadata) {
  /// 这里使用树,向上寻找,组合路径
  MetadataConstPtr k_m{};
  if (details::is_class<assets_file>(in_metadata))
    k_m = in_metadata->getParent();
  else
    k_m = in_metadata;

  FSys::path k_path{k_m->str()};
  while (k_m->hasParent()) {
    k_m    = k_m->getParent();
    k_path = FSys::path{k_m->str()} / k_path;
  }
  k_path /= core_set::getSet().get_department();
  k_path /= in_path.filename();
  setPath(in_path, k_path);
}

void assets_path::setPath(const FSys::path &in_local_path, const FSys::path &in_server_path) {
  if (!FSys::exists(in_local_path))
    throw doodle_error{"不存在文件"};
  p_local_path           = in_local_path;
  const auto k_root_path = in_local_path.root_path();
  p_lexically_relative   = in_local_path.lexically_relative(k_root_path);
  p_server_path          = in_server_path;
  p_backup_path /= FSys::add_time_stamp(in_server_path);
  DOODLE_LOG_INFO("本地路径: {}, 设置服务路径: {}, 相对路径: {} , 备份路径: {}",
                  p_local_path, p_server_path, p_lexically_relative, p_backup_path);
}

std::string assets_path::str() const {
  return fmt::format("本地路径 {}\n服务器路径 {}",
                     p_local_path,
                     p_server_path);
}
FSys::path assets_path::get_cache_path() const {
  auto k_path = core_set::getSet().get_cache_root();
  k_path /= p_lexically_relative;
  return k_path;
}
}  // namespace doodle
