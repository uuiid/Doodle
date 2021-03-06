# 这是一个maya 动作库插件

- [这是一个maya 动作库插件](#这是一个maya-动作库插件)
  - [1)首先记得加载插件](#1首先记得加载插件)
  - [2)第二部找到菜单栏](#2第二部找到菜单栏)
  - [3)设置动作库的根目录](#3设置动作库的根目录)
    - [名称注意事项](#名称注意事项)
  - [3)使用动作库](#3使用动作库)
    - [动作库主要操作](#动作库主要操作)
      - [动作库主要注意事项](#动作库主要注意事项)
        - [分类栏](#分类栏)
        - [主要窗口](#主要窗口)
        - [详细信息](#详细信息)

## 1)首先记得加载插件  
> ![加载插件](maya_motion/load_plug.jpg)

## 2)第二部找到菜单栏  
> ![打开窗口](maya_motion/open_windows.jpg)  
**在首次打开时必须设置动作库的根目录**

## 3)设置动作库的根目录
动作库的根目录必须有 **DoodleMotion.config** 这个文件, 否则无法识别, 请不要删除,在删除后会不可避免的丢失大多数信息, 同时动作库内部文件的随意改动以及代码注入会造成 **不可预知的错误和崩溃**  
在没有 **DoodleMotion.config** 时会自动创建,请尽可能的使用空目录创建动作库, 以减少不可预知的风险  
> ![设置窗口](maya_motion/setting.jpg)  
在首次打开动作库的时候请设置**制作人名称**, 以便在提交动作库的时候进行标注和搜索

### 名称注意事项
+ 😱尽量不要使用 **特殊字符** 以及 **空格** 和 **中文路径** ,以及一些 **表情符**   
   > D:/#@##@~`3%..&())😀😥😥/ **像这样的路径不可以** /&*&*(*()()_())
+ 😱路径嵌套层数不要太多,太长,这样会造成一些无法预知的问题,  
   > D:/dasdsads/sdsadsadfdf/xxxxxx/xxxxxxxxx/xxxxxxxxxxxx/xxxxxxxxxxxxxxxx/xxxxxxxxx/xxxxxxxxxxx/xxxxxxxxxxxxxxx/xxxxxxxxxxxxxx/sxsdsadsax/sadsadsa/axxxxxxxxx/xxxxxx/ **这个路径已经太长了,请不要在这里设置动作库** /fsdsadsada  
+ 😁在动作库当中是可以使用中文的, 包括制作人, 都可以使用中文  

## 3)使用动作库  
![动作库主要界面](maya_motion/main_motion.jpg)  
* 在动作库中右上方文件菜单时标准菜单, 可以打开设置或者退出
### 动作库主要操作
* 动作库的左侧是分类栏, 可以右键创建分类, 
  > ![菜单目录](maya_motion/dir_menu.jpg)   
* 动作库主要界面中可以创建动作和导入动作  
  > ![主要目录](maya_motion/main_menu.jpg)
* 动作库的最右侧是详细面板, 提供一些预览和附件信息, 
  > ![详细信息](maya_motion/attr_view.jpg)
#### 动作库主要注意事项  
##### 分类栏
 * 在权限不足的情况下, 无法编辑项目, 创建后无法更改
 * [注意事项和设置中相同](#名称注意事项)
##### 主要窗口
 * 主要窗口时动作库的主要工作窗口, 提供大部分常用功能
 * 其中创建动作的[名称输入规则](#名称注意事项)与设置相同
 * 名称长度不要超过 **64** 个字
 * **导出时注意选择根骨骼不需要要多选,只要根骨骼就行**
 * 导入就是导入fbx,基本没什么可说的
 * **更新图标和视频**在权限不足的文件夹中, **功能受限** , 根据权限会弹出相应的错误
##### 详细信息
 * 详细信息主要时提供详细的动作预览
 * 名称和注意请看[名称输入规则](#名称注意事项)
 * 信息中基本可以任意输入, 但是要注意不能输入过多的文字 **小于4096个文字** 
 * 请不要在详细信息中 **写文章(4096字以上)** 