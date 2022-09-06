# xgen 毛发解算导入 ue4

## maya 毛发解算

### 创建曲线

选中所有导向![](image_/xgen_image_00.png ':size=30*30')
使用 xgen 编辑中的工具 ![](image_/xgen_image_02.png ':size=30*30')
选中导向到曲线并创建曲线 ![](image_/xgen_image_04.png ':size=30*30')

### 创建交互式 xgen 修改

#### 创建解算系统

在大纲中选中 xgen ![](image_/xgen_image_05.png ':size=30*30')
在 xgen 菜单栏中选中**转化交互式梳理**![](image_/xgen_image_07.png ':size=30*30')
点击转化![](image_/xgen_image_08.png ':size=30*30')
选中创建的交互式毛发![](image_/xgen_image_09.png ':size=30*30')
在**xgen交互式面板**中选中创建的交互式毛发![](image_/xgen_image_10.png ':size=30*30')
在**xgen交互式面板**中点击添加修改器![](image_/xgen_image_11.png ':size=30*30')
添加**线性线条修改器**![](image_/xgen_image_12.png ':size=30*30')
在**xgen交互式面板**中选中创建的交互式修改器![](image_/xgen_image_13.png ':size=30*30')
转换面板到**属性编辑器**![](image_/xgen_image_14.png ':size=30*30')
点击属性编辑器中的**复制选项卡**![](image_/xgen_image_15.png ':size=30*30')
选中在上一步创建的**所有曲线**![](image_/xgen_image_19.png ':size=30*30')
在**线性线条修改器**的**属性编辑器**中点击**使用选定曲线作为线条**
![](image_/xgen_image_20.png ':size=30*30')
在弹出窗口中点击应用并关闭(选项不重要, 都可以)![](image_/xgen_image_21.png ':size=30*30')
之后再点击**使线条动力学化**![](image_/xgen_image_22.png ':size=30*30')
在弹出窗口点击应用并关闭![](image_/xgen_image_23.png ':size=30*30')

#### 解算毛发

选中解算核心![](image_/xgen_image_26.png ':size=30*30')
再属性编辑器中切换在解算属性标签中![](image_/xgen_image_27.png ':size=30*30')
在**解算器显示**选项中调整解算器显示![](image_/xgen_image_28.png ':size=30*30')
调整为碰撞厚度![](image_/xgen_image_29.png ':size=30*30')
注意隐藏其他毛发, 否则很难看到解算碰撞显示![](image_/xgen_image_30.png ':size=30*30')
调整**碰撞宽度偏移**![](image_/xgen_image_31.png ':size=30*30')
如果显示不正确点击**回到开始帧按钮**![](image_/xgen_image_32.png ':size=30*30')
显示成功的碰撞厚度![](image_/xgen_image_33.png ':size=30*30')
选中**解算器显示**调整显示行为, 防止解算器显示消耗资源![](image_/xgen_image_34.png ':size=30*30')
调整选项为禁用![](image_/xgen_image_35.png ':size=30*30')
点击时间栏中的播放开始查看解算效果![](image_/xgen_image_36.png ':size=30*30')
在视图中查看解算效果不错后即可停止调整参数![](image_/xgen_image_37.png ':size=30*30')
在maya中切换编辑菜单![](image_/xgen_image_42.png ':size=30*30')
切换为**fx菜单**, 用来创建被动碰撞![](image_/xgen_image_43.png ':size=30*30')
选中脸部网格体![](image_/img_2.png ':size=30*30')
打开**ncloth**菜单![](image_/xgen_image_45.png ':size=30*30')
选中创建被动碰撞对象![](image_/xgen_image_46.png ':size=30*30')
选中毛发物体![](image_/xgen_image_47.png ':size=30*30')
打开**ncache**菜单![](image_/xgen_image_251.png ':size=30*30')
选中新创建新缓存![](image_/img.png ':size=30*30')
点击**nobject**选项![](image_/img_1.png ':size=30*30')

## 导出 maya

#### 导出网格

打开缓存菜单栏![](image_/xgen_image_48.png ':size=30*30')
点击**alembic**选项![](image_/xgen_image_49.png ':size=30*30')
点击**将当前选择导出到alembic**菜单(点击后面的小方框打开选项)![](image_/xgen_image_51.png ':size=30*30')
在配置窗口中的配置是选中时间滑块 ![](image_/xgen_image_53.png ':size=30*30')
以及下方的 ![](image_/xgen_image_55.png ':size=30*30')

- 去除名称空间
- uv写入
- 写入面集
- 世界空间

文件格式选中**ogawa**![](image_/xgen_image_57.png ':size=30*30' )
最后点击应用![](image_/xgen_image_58.png ':size=30*30' )

#### 导出毛发

在maya中切换编辑菜单![](image_/xgen_image_60.png ':size=30*30' )
切换为**建模菜单**![](image_/xgen_image_61.png ':size=30*30' )
选中创建的**交互式缓存xgen**![](image_/img_3.png ':size=30*30' )
打开**生成**菜单![](image_/xgen_image_62.png ':size=30*30' )
打开**次级缓存**菜单,并点击创建新缓存![](image_/xgen_image_63.png ':size=30*30' )
打开创建缓存的配置窗口 ![](image_/xgen_image_64.png ':size=30*30' )
在配置窗口中的配置是选中时间滑块 ![](image_/xgen_image_65.png ':size=30*30' )
以及下方的写入最终宽度![](image_/xgen_image_66.png ':size=30*30' )
最后点击应用

## 导入 ue4

### 导入毛发

在ue4中创建文件架![](image_/xgen_image_67.png ':size=30*30')
重新进行命名用来作为导入使用的文件夹![](image_/xgen_image_68.png ':size=30*30')
打开新创建的文件夹![](image_/xgen_image_69.png ':size=30*30')
点击导入按钮![](image_/xgen_image_71.png ':size=30*30')
选中刚刚创建的毛发缓存![](image_/xgen_image_72.png ':size=30*30')
导入时重要的选项有(**针对maya导出的选项**)![](image_/xgen_image_73.png ':size=30*30')

- 旋转 **-90.0,0.0,180.0**
- 缩放 **-1.0,1.0,1.0**
- 以及maya时间滑块对应的开始帧和结束帧

导入完成后会出现三个ue4资产依次是![](image_/xgen_image_74.png ':size=30*30')

- 毛发本身(文件本身命名)
- 毛发指南缓存(带_guides_cache后缀)
- 毛发缓存(带_strands_cache后缀)

### 导入ue4 abc 网格体缓存

点击导入按钮![](image_/xgen_image_74.png ':size=30*30')
选中刚刚创建的网格体缓存![](image_/xgen_image_75.png ':size=30*30')
导入的重要选项有![](image_/xgen_image_76.png ':size=30*30')![](image_/xgen_image_77.png ':size=30*30')

- 导入类型为 **几何体缓存**
- 如果几何体缓存是没用顶点变化的可也选中**应用常量拓扑优化**

点击导入按钮![](image_/xgen_image_78.png ':size=30*30')
导入完成后会出现一个几何缓存体资产![](image_/xgen_image_79.png ':size=30*30')

### 在ue4中预览毛发缓存

在ue4资产管理器中点击右键,打开**动画**菜单栏选中**关卡序类**![](image_/xgen_image_80.png ':size=30*30')
双击打开刚刚创建的关卡序列资产![](image_/xgen_image_81.png ':size=30*30')
将毛发本身拖入**关卡序列**中 ![](image_/xgen_image_82.png ':size=30*30')
选中拖入的物体![](image_/img_4.png ':size=30*30')
在属性编辑器中将毛发缓存拖入到**Groom缓存属性中** ![](image_/img_5.png ':size=30*30')
在**关卡序列**中点击轨道,选中**Groom缓存**,将缓存添加到**关卡序列**![](image_/xgen_image_86.png ':size=30*30')
由于已经在属性编辑器中制定了缓存,所以添加缓存后会添加缓存条目属性![](image_/xgen_image_87.png ':size=30*30')

### 在ue4中预览几何体缓存

将导入的几何体拖拽到**关卡序列**中![](image_/xgen_image_88.png ':size=30*30')
在关卡序列中选刚刚拖拽的几何缓存体,并打开轨道菜单![](image_/xgen_image_89.png ':size=30*30')
点击**几何体缓存**选项,添加几何体缓存![](image_/xgen_image_90.png ':size=30*30')
完成添加几何体缓存![](image_/xgen_image_91.png ':size=30*30')
点击播放按钮即可进行播放![](image_/xgen_image_92.png ':size=30*30')![](image_/xgen_image_93.png ':size=30*30')
