# 开发计划

- ue4 导入使用元数据格式解析
- ue4 导入添加道具搜素
- 添加只输出abc功能
- 文件列表的排序功能

- 人名过滤
- 显示制作人名称
- 显示时间
- 不允许空标签

## maya解算类别:

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
- 解算器设置更新
    - Frame Samples 6
    - Cg Accuracy 9
    - Self Collision True
    - Shape Feature True
    - qcloth Collider 0.05

- 解算名称修改
    - 简模后缀 _proxy
    - 解算节点后缀 _cloth
    - 布料后缀 _cloth_proxy

- 解算工作 清除多边面, 解锁法线
- 检查工作
    - 简模没有蒙皮就结束
    - 有重名也不要开始
    - 有多边面也不要开始

* maya插件组合abc时取消中间物体的选择(错误修正)

## ue4

* ue4 gpu超时更新
* ue4 abc解算和fbx分开
* ue4 路人走路时的脚本固定组件开发
* 添加fbx资产替换
