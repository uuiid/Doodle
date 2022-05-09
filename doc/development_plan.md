# 开发计划

- [开发计划](#开发计划)
  - [3.4.41更新计划](#3441更新计划)
    - [软件](#软件)
    - [maya插件](#maya插件)
  - [长期计划](#长期计划)
    - [code](#code)
    - [ue4](#ue4)
  - [ue4](#ue4-1)
 
## 3.4.41更新计划

### 软件

- 文件列表排序功能
- 将json rpc 后台服务器完成

### maya插件

- 将maya插件改为后台rpc服务器
- cfx_grp 顺序
    - qlsolver
    - anim_grp
    - solver_grp
        - xxx_cloth
            - xxx_cloth_proxy
    - constraint_grp
    - collider_grp
    - deform_grp  (包裹的模型)
        - xxx_output
        - deformBase_grp
            - 包裹的base节点
    - export_grp

- 解算工作 清除多边面, 解锁法线
- 检查工作
    - 简模没有蒙皮就结束
    - 有重名也不要开始
    - 有多边面也不要开始

## 长期计划

### code

- 通过rpc 公开所有的工具功能
    - 项目设置
    - 各类工具功能
- 通过rpc 将各个软件的插件分解, 去除工具库依赖
- 添加默认项目(现在时无项目)

### ue4

- ue4 路人走路时的脚本固定组件开发
- 头发着色器制作和修改
- 头发效果提升

## ue4

* ue4 gpu超时更新
* ue4 abc解算和fbx分开
* 添加fbx资产替换

