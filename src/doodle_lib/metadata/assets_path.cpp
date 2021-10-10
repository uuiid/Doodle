//
// Created by TD on 2021/5/18.
//

#include "assets_path.h"

#include <Logger/logger.h>
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/metadata/metadata.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path)
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path_vector)

namespace doodle {
assets_path::assets_path()
    : p_local_path(),
      p_lexically_relative(),
      p_server_path(),
      p_backup_path("backup/") {
}
assets_path::assets_path(const FSys::path &in_path, const metadata_const_ptr &in_metadata)
    : p_local_path(),
      p_lexically_relative(),
      p_server_path(),
      p_backup_path("backup/") {
  if (in_metadata)
    this->set_path(in_path, in_metadata);
  else
    throw doodle_error{"空指针"};
}
const FSys::path &assets_path::get_local_path() const {
  return p_local_path;
}

const FSys::path &assets_path::get_server_path() const {
  return p_server_path;
}

const FSys::path &assets_path::get_backup_path() const {
  return p_backup_path;
}

void assets_path::set_path(const FSys::path &in_path, const metadata_const_ptr &in_metadata, bool in_using_lexically_relative) {
  if (in_using_lexically_relative) {
    auto k_prj       = in_metadata->find_parent_class<project>();
    auto k_root_path = in_metadata->find_parent_class<project>()->get_path();
    auto k_path      = in_path.lexically_relative(k_root_path);
    k_path           = FSys::path{k_prj->str()} / k_path;
    set_path(in_path, k_path);
  } else {
    /// 这里使用树,向上寻找,组合路径
    metadata_const_ptr k_m{};
    if (details::is_class<assets_file>(in_metadata))
      k_m = in_metadata->get_parent();
    else
      k_m = in_metadata;

    FSys::path k_path{k_m->str()};
    while (k_m->has_parent()) {
      k_m = k_m->get_parent();
      if (k_m->has_parent())
        k_path = FSys::path{k_m->str()} / k_path;
      else
        k_path = FSys::path{k_m->str()} / k_path;
    }
    k_path /= core_set::getSet().get_department();
    k_path /= in_path.filename();
    set_path(in_path, k_path);
  }
}

void assets_path::set_path(const FSys::path &in_local_path, const FSys::path &in_server_path) {
  if (!FSys::exists(in_local_path))
    throw doodle_error{"不存在文件"};
  p_local_path           = in_local_path;
  const auto k_root_path = in_local_path.root_path();
  p_lexically_relative   = in_local_path.lexically_relative(k_root_path);
  p_server_path          = in_server_path;
  p_backup_path /= FSys::add_time_stamp(in_server_path);
  DOODLE_LOG_INFO("本地路径: {}, 设置服务路径: {}, 相对路径: {} , 备份路径: {}",
                  p_local_path, p_server_path, p_lexically_relative, p_backup_path);
  if (!p_meta.expired())
    p_meta.lock()->saved(true);
}

std::string assets_path::str() const {
  return fmt::format("本地路径 {}\n服务器路径 {}",
                     p_local_path,
                     p_server_path);
}

void assets_path_vector::set_metadata(const metadata_ptr &in_meta) {
  leaf_meta::set_metadata(in_meta);
  for (auto &i : get()) {
    i->set_metadata(in_meta);
  }
}
assets_path_vector::path_list assets_path_vector::add_file(
    const FSys::path &in_path, bool in_using_lexically_relative) {
  path_list k_list{};
  auto k_path = new_object<assets_path>();
  // 添加基本路径
  k_path->set_metadata(p_meta.lock());
  k_path->set_path(in_path, p_meta.lock(), in_using_lexically_relative);
  k_list.push_back(k_path);

  if (ue4_project::is_ue4_file(in_path)) {
    // 添加内容路径
    k_path = new_object<assets_path>();
    k_path->set_metadata(p_meta.lock());
    k_path->set_path(in_path.parent_path() / ue4_project::Content, p_meta.lock(), in_using_lexically_relative);
    k_list.push_back(k_path);
  }

  return k_list;
}
FSys::path assets_path::get_cache_path() const {
  auto k_path = core_set::getSet().get_cache_root();
  k_path /= p_lexically_relative;
  return k_path;
}

}  // namespace doodle
