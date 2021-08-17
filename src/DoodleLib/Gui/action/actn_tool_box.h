//
// Created by TD on 2021/8/4.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include "action.h"

namespace doodle {
namespace action_arg {
class DOODLELIB_API arg_tool_box_create_ue_shot : public arg_path {
 public:
  arg_tool_box_create_ue_shot();
  std::vector<ShotPtr> shot_list;
  EpisodesPtr epsiodes;
  ProjectPtr project;
};
}  // namespace action_arg
namespace toolbox {
/**
 * @brief 这个是导出maya fbx 的动作， 需要一个多个maya文件输入
 * 
 * @details @b 导出fbx 需要选中导出的maya文件 这里是可以多项选中的 \n
 * 导出时会创建和maya文件名称相同的文件夹, 内部会放置fbx和maya文件
 * @pre 需要maya文件中的相机不要命名为 front persp side top camera 这几样
 * 并且不可以出现这几个关键字 @b 同时必须有下划线作为区分
 * 如果相机在组内， 那么将会烘培相机动画并转移到世界物体中 \n
 * @b fbx导出的骨骼必须有UE4这个关键字作为标识 这样可以导出组下方的模型
 * 
 * @warning 同时，如果maya中出现变形器， 在ue中将会丢失
 */
class DOODLELIB_API actn_export_maya
    : public action_toolbox<action_arg::arg_paths> {
 public:
  actn_export_maya();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

/**
 * @brief 这个导出视频需要多个文件夹
 * 
 * @pre 这里必须是每个文件夹中只放置序列图像, 
 * 同时 在路径中捕获 关键字 @b ep @b sc 两个关键字, 会使用这两关键字进行解析,寻找到时添加到视频水印,
 * 没有寻找到的话没有这一部分水印
 */
class DOODLELIB_API actn_create_video
    : public action_toolbox<action_arg::arg_paths> {
  std::vector<ImageSequencePtr> p_image;

 public:
  actn_create_video();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

/**
 * @brief 连接多个视频
 * 
 * @pre 连接视频前需要仔细检查是否有空视频， 空视频会导致出错
 */
class DOODLELIB_API actn_connect_video
    : public action_toolbox<action_arg::arg_paths> {
  video_sequence_ptr p_video_sequence;

 public:
  actn_connect_video();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

/**
 * @brief 添加ue项目
 * 
 * @pre 创建ue关卡需要 @ref doodle_windwos_setting "设置" 中的ue4路径
 * 如果为空会失败
 * 同时如果没有创建成功请删除一下配置文件和缓存
 * 
 * @bug 在创建完成后ue4会有崩溃出现， 但是并不影响内容
 */
class DOODLELIB_API actn_ue4_shot_episodes
    : public action_toolbox<action_arg::arg_tool_box_create_ue_shot> {
  ue4_project_ptr p_ptr;

 public:
  actn_ue4_shot_episodes();
  bool is_async() override;
  using arg = action_arg::arg_tool_box_create_ue_shot;

 protected:
  long_term_ptr run() override;
};

class DOODLELIB_API actn_qcloth_sim_export
    : public action_toolbox<action_arg::arg_paths> {
 public:
  actn_qcloth_sim_export();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

}  // namespace toolbox
}  // namespace doodle
