
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/core/app_base.h>

namespace doodle {
/**
 * @brief
 * # 主工具
 * * 辅助安装maya插件
 * * 辅助安装ue4插件
 * * 清除ue4缓存
 * * 修改ue4缓存位置
 * * 批量导出fbx
 * * 批量进行布料解算
 * * 创建ue4关卡
 * * 导入ue4 fbx和abc文件
 * * 创建拍屏并合成水印
 * * 连接拍屏
 * * 模型库工具（正在开发）
 *
 * # ue工具
 * * 批量修正材质名称和材质属性
 * * 导入fbx和abc
 * * 传递材质
 * *
 *
 * @todo ue4 导入使用元数据格式解析
 * @todo ue4 导入添加道具搜素
 * @todo 创建检查maya 场景工具架
 * @todo 添加只输出abc功能
 *
 * @todo ue4 gpu超时更新
 * @todo ue4 abc解算和fbx分开
 * @todo ue4 火光复制功能添加
 * @todo ue4 路人走路时的脚本固定组件开发
 * @todo @b 火光 属性记录动画帧元数据
 * @todo @b 灯光 球属性记录动画帧元数据
 * @todo ue4 灯光配置父子关系添加
 *
 */
class DOODLELIB_API app : public app_command_base {
 protected:
  win::wnd_class p_win_class;

  std::shared_ptr<win::d3d_device> d3d_deve;
  bool p_show_err;

 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit app(const win::wnd_instance& in_instance);
  explicit app();
  ~app() override;
  win::wnd_handle p_hwnd;
  ::ID3D11Device* d3dDevice;
  ::ID3D11DeviceContext* d3dDeviceContext;

  static app& Get();
  bool valid() const override;

  void loop_one() override;

  virtual void hide_windows();
  virtual void show_windows();

 protected:
  virtual void load_windows();
  void load_back_end() override;
};
}  // namespace doodle
