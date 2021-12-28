//
// Created by TD on 2021/9/19.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>
namespace doodle {

/**
 * @brief maya工具类
 * @image html comm_maya_tool.jpg maya工具窗口
 *
 * @warning 在导出过程中关闭软件或者强制结束任务会产生不可预置的问题
 *
 * @li @b 总项 \n
 * maya文件选择时，对于解算和导出来说是公用的路径，
 *
 * @li @b 解算子项 \n
 * 解算时有解算资源所在路径选项的要求， 这个是必须填写的项目，
 * 在使用只解算时是不会替换解算引用的
 * @warning
 *  * 需要 在解算文件中具有 _cloth_proxy 后缀的解算体, 我们使用它设置缓存目录和创建缓存文件
 *  * 需要 maya版本为2019
 *  * 需要 有 UE4 后缀的组, 使用这个组来输出abc文件
 *  * 需要 解算从950开始
 *  * 需要帧率时25fps
 *  * 需要 cloth 后缀的 物体来设置解算缓存文件的路径 , 他的形状节点必须时 clothShape后缀(默认即可)
 *  * 最好有一台摄像机用来创建拍屏
 *  * 最好具有标记元数据, 使用这个元数据来确认解算的人物
 *  * 最好有 _skin_proxy 结尾的物体, 我们使用他来校验有效性
 *  * 只解算和导出选项是不兼容的
 *
 * @li @b 导出fbx子项 \n
 * 导出fbx时是按照保存是引用保存时的状态加载的，
 * 如果勾选直接加载所有引用的话， 会不查看保存状态， 直接加载所有引用
 *
 *
 */
class DOODLELIB_API comm_maya_tool : public command_base {
  FSys::path p_cloth_path;
  std::string p_text;
  std::vector<FSys::path> p_sim_path;
  bool p_only_sim;
  bool p_use_all_ref;

 public:
  comm_maya_tool();
  bool is_async() override;
  bool render() override;
};

/**
 * @brief 创建视频工具类
 * @image html comm_create_video.jpg 创建视频工具
 *
 * @li @b 输出文件夹 \n
 * 输出文件夹指定了输出路径,这个是必填项目
 *
 * @li @b 图片序列 :\n
 * 这个可以 @b 选择图片 序列和 @b 选择文件夹， \n
 * @note 这里文件夹中的图片将全部载入合成视频 \n
 * 同时可以多次选中添加到合成队列中， 并且一起合成 \n
 * 需要清除时， 点击清除按钮 \n
 *
 * @li @b 清除:
 * 清除整个列表
 * @li @b 创建视频:
 * 直接将整个队列中的文件连接为多个视频
 *
 * @li @b 视频序列 \n
 * 选择视频, 后点击连接视频进行连接
 *
 */
class DOODLELIB_API comm_create_video : public command_base {
  struct image_paths {
    std::vector<FSys::path> p_path_list;
    FSys::path p_out_path;
    std::string p_show_name;
    bool use_dir;
  };
  std::vector<FSys::path> p_video_path;
  std::vector<image_paths> p_image_path;
  std::shared_ptr<std::string> p_out_path;

 public:
  comm_create_video();
  bool is_async() override;
  bool render() override;
};
/**
 * @brief 将abc和fbx导入ue4 项目中
 * @image html comm_import_ue_files.jpg 导入ue
 * @li @b ue项目 选择ue4项目
 * @li @b 选择导入 选择ue4 项目中
 *
 */
class DOODLELIB_API comm_import_ue_files : public command_base {
  FSys::path p_ue4_prj;
  std::shared_ptr<std::string> p_ue4_show;

  std::vector<FSys::path> p_import_list;

 public:
  comm_import_ue_files();
  bool is_async() override;
  bool render() override;
};

}  // namespace doodle
