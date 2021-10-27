//
// Created by TD on 2021/5/18.
//

#include "assets_path.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/rpc/rpc_trans_path.h>
#include <logger/logger.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path)
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path_vector)

namespace doodle {
assets_path::assets_path()
    : p_local_path(),
      p_lexically_relative(),
      p_server_path(),
      p_backup_path("backup/") {
}
assets_path::assets_path(const FSys::path &in_path, const entt::handle &in_metadata)
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

void assets_path::set_path(const FSys::path &in_path, const entt::handle &in_metadata, bool in_using_lexically_relative) {
  if (in_using_lexically_relative) {
    auto k_prj       = in_metadata.get<tree_relationship>().find_parent_class<project>();
    auto k_root_path = in_metadata.get<tree_relationship>().find_parent_class<project>()->get_path();
    auto k_path      = in_path.lexically_relative(k_root_path);
    k_path           = FSys::path{k_prj->str()} / k_path / in_path.filename();
    set_path(in_path, k_path);
  } else {
    /// 这里使用树,向上寻找,组合路径

    auto k_reg = g_reg();

    entt::handle k_m{};
    if (in_metadata.any_of<assets_file>())
      k_m = in_metadata.get<tree_relationship>().get_parent_h();
    else
      k_m = in_metadata;



    FSys::path k_path{k_m.get_or_emplace<to_str>().get()};
    while (k_m.get<tree_relationship>().has_parent()) {
      k_m = k_m.get<tree_relationship>().get_parent_h();
      if (k_m.get<tree_relationship>().has_parent())
        k_path = FSys::path{k_m.get<to_str>().get()} / k_path;
      else
        k_path = FSys::path{k_m.get<to_str>().get()} / k_path;
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
}

std::string assets_path::str() const {
  return fmt::format("本地路径 {}\n服务器路径 {}",
                     p_local_path,
                     p_server_path);
}

command_ptr assets_path_vector::add_file(
    const FSys::path &in_path, bool in_using_lexically_relative) {
  command_ptr k_comm{new_object<comm_files_up>()};

  auto k_path = new_object<assets_path>();
  get().push_back(k_path);

  auto k_h = make_handle(*this);
  if (ue4_project::is_ue4_file(in_path)) {
    k_h.get<assets_file>().set_file_type(assets_file_type::ue4_prj);
    // 添加基本路径(ue4 prj 路径)
    k_path->set_path(in_path, k_h, in_using_lexically_relative);
    // 添加内容路径
    k_path = new_object<assets_path>();
    k_path->set_path(in_path.parent_path() / ue4_project::Content, k_h, in_using_lexically_relative);
    get().push_back(k_path);
  }

  if (image_sequence::is_image_sequence(FSys::list_files(in_path.parent_path()))) {
    k_h.get<assets_file>().set_file_type(assets_file_type::ue4_prj);
    // 添加文件的父路径, 序列文件夹
    k_path->set_path(in_path.parent_path(), k_h, in_using_lexically_relative);
    k_comm = new_object<comm_file_image_to_move>();
  }
  return k_comm;
}

void assets_path_vector::add_file_raw(const FSys::path &in_path, bool in_using_lexically_relative) {
  auto k_path = new_object<assets_path>();
  get().push_back(k_path);
  k_path->set_path(in_path, make_handle(*this), in_using_lexically_relative);
}

rpc_trans_path_ptr_list assets_path_vector::make_up_path() const {
  rpc_trans_path_ptr_list k_list{};
  for (auto &i : paths)
    k_list.emplace_back(std::make_unique<rpc_trans_path>(i->get_local_path(), i->get_server_path(), i->get_backup_path()));
  return k_list;
}
FSys::path assets_path::get_cache_path() const {
  auto k_path = core_set::getSet().get_cache_root();
  k_path /= p_lexically_relative;
  return k_path;
}

}  // namespace doodle
