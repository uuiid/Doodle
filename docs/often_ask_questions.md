# 常问问题

## 基本的

### 在哪里查看功能和工作原理

* 功能查看 @ref doc/schedule.md "描述"
* 软件工作原理 具体查看文档中的类描述

### 软件中的新项目是什么

* 你可以认为是一个储存文件的目录， 这个文件记录了软件本身产生的数据

### 如何查询计算时间

* 按照选中的实体， 将他们按照时间排序， 然后计算两者的时间差**选中的物体必须有路径组件的存在才行**
  > 计算路径方式:
  > 项目路径(绝对路径 E:/test/biao 保存的文件的路径) 和 拖入的路径 -> 计算相对路径 -> 相对路径为空
  > E:/file/image.png 和 F:/tmp 相对 -> 空
  > E:/file/image.png 和 E:/tmp 相对 -> ../image.png

### 如果添加 ue4 人群 ai

- 创建一个继承 DoodleCurveCrowd 的蓝图类
- 将 蓝图类中的骨骼网格体设置为需要的网格体, 将动画资产指定为骨骼网格体对应的混合动画

### cmake c++ 工程

- 配置 -> 配置成功
- 构建 -> 构建完成 doodleexe.exe 文件
- 安装

### vscode

- cmake 命令行使用
- msvc 环境配置
- cmd 脚本编写
- cmake presste.json 使用
- task.json 编写
- C++ debug lanuch.json 编写
- c_cpp_properties.json 编写 (指导c++ 智能编辑使用)

### clion 

