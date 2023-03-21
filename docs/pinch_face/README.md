# UE5捏脸系统使用说明文档
## 工具准备
- 虚幻引擎版本：5.1
- 安装
  - 打开Doodle
  - 工具---->安装UE4插件
![](12.jpg)
  - 窗口---->设置窗口，填写ue路径，填写ue版本(5.1)
![](13.jpg)
## 视口说明 
![](9.png)
- 骨骼编辑器
  - Edit按钮：点击按钮切换到对应的骨骼进行修改
  - 双击Add_Bone:可以对名称进行修改
![](10.png)
- 曲线编辑器
  - xx.TranslationCurve.X :表示xx(骨骼)沿X轴平移
  - xx.TranslationCurve.Y :表示xx(骨骼)沿Y轴平移
  - xx.TranslationCurve.Z :表示xx(骨骼)沿Z轴平移
  - xx.ScaleCurve.X:表示xx(骨骼)沿X轴缩放
  - xx.ScaleCurve.Y:表示xx(骨骼)沿Y轴缩放
  - xx.ScaleCurve.Z:表示xx(骨骼)沿Z轴缩放
  - xx.RotationCurve.X:表示xx(骨骼)沿X轴旋转
  - xx.RotationCurve.Y:表示xx(骨骼)沿Y轴旋转
  - xx.RotationCurve.Z:表示xx(骨骼)沿Z轴旋转
## 使用说明
- 右键----> Doodle Character---->Create Character
![](1.png)
- 选择自己需要的骨骼---->OK
![](2.png)
- 双击打开刚刚创建的插件
![](3.png)
- 在骨骼编辑器中右键---->Add Classify(创建根目录)
![](4.png)
- 在我们刚刚创建出来的根目录下右键---->Add Classify(创建子目录)
![](5.png)
- 点击子目录---->右键---->Binding---->选择自己需要修改的骨骼
![](6.png)
- 在左下侧曲线编辑器中选择需要的修改的曲线
  - 在示例中，我们选择的是修改肩膀的平移曲线
  - 右键---->添加关键(可以添加多个关键帧) 
![](7.png) 
- 修改关键帧的位置即可调整该骨骼修改的的幅度，移动滑块即可修改该骨骼
![](8.png) 




