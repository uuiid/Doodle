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

class DOODLELIB_API arg_tool_box_sim_qcloth : public arg_paths {
 public:
  arg_tool_box_sim_qcloth();
  FSys::path qcloth_assets_path;
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
  MayaFilePtr p_maya;

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
/**
 * @brief 
 * 
 * 批量解算qcloth 文件
 * 
 * @li 选择文件后会自动替换qcloth 模板文件， 
 * @li 他们的文件名称必须 *_rig --> *_cloth 是这种对应格式化, 大小写不可替换 
 * @li 在文件中 解算体必须以*_cloth_proxy后缀结束 
 * @li 更新post 姿势物体和解算体的对应关系必须是 *_cloth_proxy --> *_skin_proxy  
 * @li 在使用时注意maya 版本 请在 @ref doodle_windwos_setting 中设置 为 2019 的版本 \n 
 * 例如: C:/Program Files/Autodesk/Maya2019/bin
 * 
 * 
 * @warning @li 引用的文件必须和动画文件帧率相同，比如一个引用是24帧每秒， 动画是25 ，就会卡死 \n
 * @li 解算帧开始必须为950
 * @li 路径中出现中文有可能会出错误, 请注意
 * @li 插件名称不要更改 必须是 qualoth_2019_x64
 * 
 * 
 */
class DOODLELIB_API actn_qcloth_sim_export
    : public action_toolbox<action_arg::arg_tool_box_sim_qcloth> {
  MayaFilePtr p_maya;

 public:
  actn_qcloth_sim_export();
  bool is_async() override;
  using arg = action_arg::arg_tool_box_sim_qcloth;

 protected:
  long_term_ptr run() override;
};

}  // namespace toolbox
}  // namespace doodle
