# ue4 groom 影子闪烁解决

## 命令行变量配置

这个`r.HairStrands.Voxelization.Virtual.VoxelWorldSize 0`用来配置命令行可去除一部分
[参考视频](https://www.youtube.com/watch?v=IpSVbYveTQA ':ignore')  
在配置文件中需要配置`r.RayTracing.Shadows=True`和`r.RayTracing.Skylight=True`用来**启用光追踪阴影**
在配置项目设置中也可以设置这两项  
在平行光中,需要设置**投射深度阴影**打开, 以及**深度阴影图层分布**这个值需要调整,在**投射光线追踪阴影**属性中, 不可以使用禁用选项


