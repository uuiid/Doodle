//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_core/core/core_set.h>

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {
/**
 * @addtogroup 设置窗口
 * @image html doodle_setting.jpg 设置窗口
 *
 * @li @b 部门 所在部门 (默认为 None_)
 * @li @b 用户 制作者的名称， 可以使用任意中英文字符，
 * @li @b 缓存 缓存文件路径是不可更改的，只是用来作为提示
 * @li @b 文档 文档路径和缓存路径都是不可更改的，
 * @li @b maya路径 maya路径是在使用有maya 参与处理文件时， 启动maya 的路径
 * @warning 在这里尽量不要出现中文 \n
 *
 * maya的可执行文件应该在这个目录里面
 * @image html maya_help_find_path.jpg maya路径寻找
 * 在这里可以使用快捷方式右键 @b 打开文件所在位置 来找到这个路径
 * 同时， 如果安装在默认路径的话， 软件会自动寻找到
 *
 *
 * @li @b ue路径 和maya路径一样的  \n
 * @warning 在这里尽量不要出现中文 \n
 * @pre 使用 @b epic @b games 安装，那么软件会自动按照版本寻找
 * 这里使用ue 安装目录是 下方几个目录的上一级
 * @code
 *  /.egstore
 *  /Engine
 *  /FeaturePacks
 *  /Samples
 *  /Templates
 * @endcode
 *
 * ue路径比较难以寻找，
 * 请仔细填写版本 \n
 * @li @b ue版本 这里的般的要和安装路径中的版本相同 \n
 * @warning 和安装路径中的版本相同
 *
 *
 */

/**
 * @brief 设置主窗口
 *
 */
class setting_windows : public base_windows<dear::Begin, setting_windows> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  setting_windows();
  ~setting_windows() override;

  constexpr static std::string_view name{gui::config::menu_w::setting};
  const std::string& title() const override;
  void init();
  void render();
  void save();
};

}  // namespace doodle::gui
