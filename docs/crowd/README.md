# ue人群制作

## 具有控制头部朝向的人群

> [!NOTE]
> 这个doodle 的ue插件版本必须高于**v3.5.182**

### 创建post

#### 创建多个post帧

需要创建多个静止姿势为瞄准偏移做准备![test](image_/crowd_000.png ':size=30*30')  
首先选中导入的fbx动画序列![](image_/crowd_001.png ':size=30*30'),_由于改动后需要保存, 可也先复制出一个_,
使用右键打开菜单后选中**在新窗口中打开**![](image_/crowd_002.png ':size=30*30'),
将游标移动到序列中想要的post帧![](image_/crowd_004.png ':size=30*30'),
在**游标上**右键打开动作菜单![](image_/crowd_005.png ':size=30*30'),选中**序列编辑**中的第一项,一般是**
移除0到xx之间的帧**
移除后继续在**游标上**![](image_/crowd_007.png ':size=30*30')选中**序列编辑**中的第一项,一般是**移除0到xx之间的帧**,
之后这个序列会成为**只有一帧的post序列**,在修改完成后需要**保存**![](image_/crowd_008.png ':size=30*30'),
在需要的post序列中重复这这个过程, 直到创建出了足够的post,
这是创建后的效果![](image_/crowd_014.png ':size=30*30')![](image_/crowd_017.png ':size=30*30')
![](image_/crowd_019.png ':size=30*30')

#### 调整post属性

选中创建的post![](image_/crowd_027.png ':size=30*30'),右键弹出菜单动作,在**资产操作**菜单中,
点击**通过属性矩阵进行批量编辑**![](image_/crowd_030.png ':size=30*30'),
打开**属性矩阵编辑对话框**, 在属性编辑右侧中打开**additivesetting**子菜单![](image_/crowd_031.png ':size=30*30'),
调整前三个属性值,

- 第一个改为网格体空间
- 第二个更改为选择的动画帧
  ![](image_/crowd_035.png ':size=30*30')
- 第三个更改为**动画post**, ![](image_/crowd_036.png ':size=30*30'),
  这个值必须是相同网格体,并且带有一段动画的值, 不可以是创建出来的post,

### 创建瞄准偏移

选中导入时网格体的骨骼![](image_/crowd_047.png ':size=30*30'),
并右键弹出操作菜单后打开**创建子菜单**![](image_/crowd_048.png ':size=30*30'),选中**瞄准偏移**,
将创建的**瞄准偏移**![](image_/crowd_049.png ':size=30*30')双击打开![](image_/crowd_050.png ':size=30*30'),
将右侧的资产编辑器中创建的post拖入做标轴视图中![](image_/crowd_052.png ':size=30*30'),依次拖入所有的post,
在轴视图中的不同位置代表了![](image_/img_1.png ':size=30*30'),代表了不同参数的混合, 可也拖动绿色的按钮进行预览,
在**轴设置**(**Axis Setting**)窗口中, 设置轴名称为**xy**和**z**名称,并将最大值和最小值重新设置为 60 和 -60,
这个值可也以调整,但是必须保证**绝对值相等**, 并且**绝对值不可大于180**

> [!NOTE]
> 其中xy轴向控制人物左右转向, z轴控制人物上下转向, 请将**静止post**放置到适合的点位上, 不然会出现控制朝向发生错误的问题

### 创建动画蓝图

在资源管理器中右键弹出操作菜单, 打开动画子菜单,并且选中**动画蓝图**进行创建![](image_/crowd_066.png ':size=30*30'),
在弹出菜单中父类选中插件中的 **DoodleAnimInstance**类![](image_/crowd_067.png ':size=30*30'),
目标骨骼选中创建post动画引用的骨骼![](image_/crowd_075.png ':size=30*30'),
双击打开刚刚创建的动画蓝图, 这个是最终效果![](image_/crowd_076.png ':size=30*30'),
选中动画图表并进行编辑![](image_/crowd_084.png ':size=30*30'),
在窗口中打开**资产游览器**![](image_/img.png ':size=30*30')
将**资产游览器**中刚刚创建的瞄准偏移拖入**
动画图表**![](image_/crowd_086.png ':size=30*30')![](image_/crowd_087.png ':size=30*30'),
并将**静止post**从**资产游览器**![](image_/crowd_089.png ':size=30*30')拖入**动画图表**
![](image_/crowd_090.png ':size=30*30'),
同时将**其他另一个动画**(用来和**静止post**进行混合播放的动画)从**资产游览器**
![](image_/img_6.png ':size=30*30')拖入 **动画图表**
![](image_/img_7.png ':size=30*30'),
在**动画图表**中右键点击弹出**创建操作菜单**,输入**blends**,创建**混合节点**,将**静止post**中的输出链接到**混合节点**
的**A**输入端,将**其他另一个动画**中的输出链接到**混合节点**的**B**输入端![](image_/img_8.png ':size=30*30'),
搜索变量**获取 Random Attr**,并点击后创建出节点![](image_/img_9.png ':size=30*30'),
将输出连接到混合节点的**alpha**输入端,最后将**混合节点**的输出连接到**瞄准偏移**的**base post**
输入端![](image_/img_10.png ':size=30*30'),
在**动画图表**中右键点击弹出**创建操作菜单**,输入**direction**,
搜索变量**获取Direction Attr XY**,**获取Direction Attr Z**条目,![](image_/crowd_101.png ':size=30*30')
并点击后创建出这个两个条目![](image_/img_12.png ':size=30*30'),并将输出从 **获取Direction Attr XY** 连接到
**xy**输入端, 输出从 **获取Direction Attr Z** 连接到 **z**输入端![](image_/img_11.png ':size=30*30'),
最终将瞄准偏移的输出连接到**输出姿势**的输入端 ![](image_/img_13.png ':size=30*30'),
并点击**编译**和**保存**![](image_/img_2.png ':size=30*30'),
最终结果完成![](image_/img_14.png ':size=30*30')

### 创建Character类

在资源管理器中右键弹出操作菜单,选中**蓝图类**进行创建![](image_/crowd_125.png ':size=30*30'),
其中类的选择是Character(**角色**)类![](image_/img_15.png ':size=30*30'),
并双击打开刚刚创建的类![](image_/crowd_128.png ':size=30*30'),
在左侧拖入**骨骼物体**![](image_/crowd_130.png ':size=30*30'),并选中**视口窗口**
![](image_/crowd_131.png ':size=30*30'),
将**人物和箭头方向对齐**![](image_/crowd_132.png ':size=30*30'),之后选中拖入的**
骨骼网格体**![](image_/crowd_130.png ':size=30*30'),
将动画类更改为上一步创建的类![](image_/crowd_136.png ':size=30*30'), 并点击**
编译按钮**![](image_/crowd_141.png ':size=30*30')

### 在导演蓝图中创建委托

将上一步创建的**蓝图类**拖入**Sequencer**中![](image_/crowd_148.png ':size=30*30'),点击**
轨道按钮**![](image_/crowd_149.png ':size=30*30'),
弹出动作菜单,并点击拖入的骨骼物体,将骨骼物体添加到轨道中![](image_/crowd_150.png ':size=30*30'),
在骨骼物体的右侧再次点击**轨道按钮**, ![](image_/crowd_151.png ':size=30*30'),选中第四步创建的**
动画实例**![](image_/crowd_153.png ':size=30*30'),
在动画实例中再次点击**轨道按钮**![](image_/crowd_154.png ':size=30*30'),在轨道菜单中,
选择事件子菜单**Doodle Cluster**选项添加自定义轨道![](image_/img_4.png ':size=30*30'),
之后点击右键![](image_/img_5.png ':size=30*30'),调整 **doodle lock at object** 属性为看向目标



