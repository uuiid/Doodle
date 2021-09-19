//
// Created by TD on 2021/8/4.
//

#include "actn_tool_box.h"

#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/threadPool/ThreadPool.h>

namespace doodle {
namespace action_arg {
arg_tool_box_create_ue_shot::arg_tool_box_create_ue_shot()
    : arg_path(),
      shot_list() {
}

arg_tool_box_sim_qcloth::arg_tool_box_sim_qcloth()
    : arg_paths(),
      qcloth_assets_path() {
}

arg_tool_box_ue4_import::arg_tool_box_ue4_import()
    : arg_paths(),
      ue4_project() {
}
}  // namespace action_arg
namespace toolbox {

actn_export_maya::actn_export_maya()
    : action_toolbox<action_arg::arg_paths>(),
      p_maya() {
  p_name = "导出fbx";
}
long_term_ptr actn_export_maya::run() {
  auto k_term = new_object<long_term>();
  p_date      = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return k_term;
  }
  p_maya = new_object<MayaFile>();
//  std::vector<long_term_ptr> k_list{};
//  std::transform(p_date.date.begin(), p_date.date.end(), std::back_inserter(k_list),
//                 [this](const auto& in) {
//                   return p_maya->exportFbxFile(in);
//                 });
//  k_term->forward_sig(k_list);
  return k_term;
}
bool actn_export_maya::is_async() {
  return true;
}

actn_create_video::actn_create_video()
    : action_toolbox<action_arg::arg_paths>(),
      p_image() {
  p_name = "创建视频";
}
bool actn_create_video::is_async() {
  return true;
}
long_term_ptr actn_create_video::run() {
  auto k_term = new_object<long_term>();
  p_image.clear();

  p_date = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return k_term;
  }

  auto k_video = new_object<ImageSequence>();
  std::transform(p_date.date.begin(), p_date.date.end(), std::back_inserter(p_image),
                 [](const FSys::path& in) {
                   auto k_shot = new_object<Shot>();
                   k_shot->analysis(in);
                   auto k_eps = new_object<Episodes>();
                   k_eps->analysis(in);

                   auto k_image = new_object<ImageSequence>(in, "");
                   k_image->set_shot_and_eps(k_shot, k_eps);
                   return k_image;
                 });
  std::vector<long_term_ptr> k_list;
  std::transform(p_image.begin(), p_image.end(), std::back_inserter(k_list),
                 [](const ImageSequencePtr& in) {
                   return long_term_ptr{};
                 });

  k_term->forward_sig(k_list);

  return k_term;
}

actn_connect_video::actn_connect_video()
    : action_toolbox<action_arg::arg_paths>(),
      p_video_sequence() {
  p_name = "连接视频";
}
bool actn_connect_video::is_async() {
  return true;
}
long_term_ptr actn_connect_video::run() {
  auto k_term = new_object<long_term>();
  p_date      = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return k_term;
  }

  p_video_sequence = new_object<VideoSequence>(p_date.date);
//  k_term->forward_sig(p_video_sequence->connectVideo_asyn());

  return k_term;
}

actn_ue4_shot_episodes::actn_ue4_shot_episodes()
    : action_toolbox<action_arg::arg_tool_box_create_ue_shot>() {
  p_name = "创建ue4关卡";
}
bool actn_ue4_shot_episodes::is_async() {
  return true;
}
long_term_ptr actn_ue4_shot_episodes::run() {
  auto k_term = new_object<long_term>();
  p_date      = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return k_term;
  }

  p_ptr = new_object<Ue4Project>(p_date.date);
  //  k_term->forward_sig(p_ptr->create_shot_folder_asyn(p_date.shot_list));
  //
  //  return k_term;
  return {};
}
actn_qcloth_sim_export::actn_qcloth_sim_export()
    : p_maya() {
  p_name = "进行qcloth批量解算";
}
bool actn_qcloth_sim_export::is_async() {
  return true;
}
long_term_ptr actn_qcloth_sim_export::run() {
  auto k_term = new_object<long_term>();
  p_date      = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return k_term;
  }
  p_maya = new_object<MayaFile>();
  std::vector<long_term_ptr> k_list{};
  std::transform(p_date.date.begin(), p_date.date.end(), std::back_inserter(k_list),
                 [this](const auto& in) {
                   auto ptr                = new_object<MayaFile::qcloth_arg>();
                   ptr->sim_path           = in;
                   ptr->qcloth_assets_path = p_date.qcloth_assets_path;
                   ptr->only_sim           = p_date.is_sim;
                   return long_term_ptr{};
                 });
  k_term->forward_sig(k_list);
  return k_term;
}

actn_ue4_import_files::actn_ue4_import_files() {
  p_name = "批量导入ue4文件";
}

bool actn_ue4_import_files::is_async() {
  return true;
}

long_term_ptr actn_ue4_import_files::run() {
  p_date = sig_get_arg().value();
  if (p_date.is_cancel) {
    cancel("取消");
    return {};
  }

  p_ue = new_object<Ue4Project>(p_date.ue4_project);
  std::vector<FSys::path> k_list;
  std::copy_if(p_date.date.begin(), p_date.date.end(), std::back_inserter(k_list),
               [this](const FSys::path& in_path) { return p_ue->can_import_ue4(in_path); });

  //  auto k_term = p_ue->import_files_asyn(k_list);
  //  p_term      = k_term;
  //  return k_term;
  return nullptr;
}
}  // namespace toolbox

}  // namespace doodle
