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
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/rpc/rpc_trans_path.h>
#include <logger/logger.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_path_vector)

namespace doodle {

assets_path_vector::assets_path_vector() {}

const FSys::path &assets_path_vector::get_local_path() const {
  return p_local_path;
}

const FSys::path &assets_path_vector::get_server_path() const {
  return p_server_path;
}

const FSys::path &assets_path_vector::get_backup_path() const {
  return p_backup_path;
}

bool assets_path_vector::make_path() {
  auto k_h = make_handle(*this);
  return make_path(k_h);
}

bool assets_path_vector::make_path(const entt::handle &in_metadata) {
  FSys::path k_path{};
  k_path /= in_metadata.get<root_ref>().root_handle().get<project>().str();
  if (in_metadata.all_of<season>())
    k_path /= in_metadata.get<season>().str();
  if (in_metadata.all_of<episodes>())
    k_path /= in_metadata.get<episodes>().str();
  if (in_metadata.all_of<shot>())
    k_path /= in_metadata.get<shot>().str();

  if (in_metadata.all_of<assets>())
    k_path /= in_metadata.get<assets>().str();

  k_path /= core_set::getSet().get_department();
  p_server_path = k_path;
  p_backup_path /= FSys::add_time_stamp(k_path);
  DOODLE_LOG_INFO("设置服务路径: {}, 备份路径: {}",
                  p_server_path, p_backup_path);
  return true;
}

bool assets_path_vector::make_path(const entt::handle &in_metadata, const FSys::path &in_path) {
  FSys::path k_path{};
  auto &k_prj      = in_metadata.get<root_ref>().root_handle().get<project>();
  auto k_root_path = k_prj.get_path();
  k_path           = in_path.lexically_relative(k_root_path);
  k_path           = FSys::path{k_prj.str()} / (k_path.empty() ? in_path.filename() : k_path);

  p_server_path    = k_path;
  p_backup_path /= FSys::add_time_stamp(k_path);
  DOODLE_LOG_INFO("设置服务路径: {}, 备份路径: {}",
                  p_server_path, p_backup_path);
  return true;
}

command_ptr assets_path_vector::add_file(
    const FSys::path &in_path) {
  command_ptr k_comm{new_object<comm_files_up>()};

  auto k_h = make_handle(*this);
  if (ue4_project::is_ue4_file(in_path)) {
    p_local_path = in_path.parent_path();
    k_h.get<database>().set_meta_type(metadata_type::ue4_prj);
    // 添加基本路径(ue4 prj 路径)
    p_file_list.push_back(in_path.filename());
    // 添加内容路径
    p_file_list.push_back(ue4_project::Content);
  }

  if (image_sequence::is_image_sequence(FSys::list_files(in_path.parent_path()))) {
    p_local_path = in_path.parent_path();
    k_h.get<database>().set_meta_type(metadata_type::movie);
    // 添加文件的父路径, 序列文件夹
    p_file_list.push_back(in_path.parent_path().filename());
    k_comm = new_object<comm_file_image_to_move>();
  }
  return k_comm;
}

void assets_path_vector::add_file_raw(const FSys::path &in_path) {
  p_file_list.push_back(in_path.filename());
}

rpc_trans_path_ptr_list assets_path_vector::make_up_path() const {
  rpc_trans_path_ptr_list k_list{};

  if (p_file_list.size() != p_local_paths.size())
    throw doodle_error{"错误的上传路径"};
  for (size_t i = 0; i < p_file_list.size(); ++i) {
    k_list.emplace_back(std::make_unique<rpc_trans_path>(p_local_paths[i], get_server_path() / p_file_list[i], get_backup_path()));
  }

  return k_list;
}

rpc_trans_path_ptr_list assets_path_vector::make_down_path(const FSys::path &in_down_path) const {
  rpc_trans_path_ptr_list k_list{};
  for (auto &i : p_file_list) {
    k_list.emplace_back(std::make_unique<rpc_trans_path>(in_down_path / i, get_server_path() / i, get_backup_path()));
  }
  return k_list;
}

FSys::path assets_path_vector::get_cache_path() const {
  auto k_path = core_set::getSet().get_cache_root();
  k_path /= p_server_path;
  return k_path;
}

std::vector<FSys::path> assets_path_vector::list() {
  std::vector<FSys::path> k_l;
  std::transform(p_file_list.begin(), p_file_list.end(), std::back_inserter(k_l),
                 [this](auto &p) {
                   return get_server_path() / p;
                 });
  return k_l;
}

}  // namespace doodle
