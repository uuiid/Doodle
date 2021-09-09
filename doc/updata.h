#pragma once

/**
 * @page updata_log 更新日志
 * @tableofcontents
 * @section updata_log_3 版本3
 *
 * @subsection updata_log_309 版本3.0.9
 *
 * @li 更新maya自动解算组件： 导出拍屏位置为mov文件夹
 * @li 更新maya自动解算组件： 拍屏优先使用quicktime h264解码
 * @li 更新maya自动解算组件： 保存文件位置为文件名称所在文件夹
 * @li 更新maya自动解算组件： 拍屏开始时间确定为1001开始帧
 * @li 更新ue4 批量导入功能： 导入时启用多进程导入
 *
 * @subsection updata_log_3010 版本3.0.10
 *
 * @li 更新maya插件：添加maya 动画解算标记工具
 * @li 更新maya插件：修改maya插件安装位置安装冲突
 * @li 更新maya插件：修改maya场景清理工具，删除maya文件健康处理脚本功能
 * @li 更新maya插件：导出fbx 相机工具更改
 * @li 更新maya插件：更改手动导出abc时的方式
 * @li 更新maya插件：添加abc手动导出时选择项目对话框 \n
 * @li 更新maya自动解算组件：maya 解算拍屏工具定义渲染属性修改
 * @li 更新maya自动解算组件：添加maya解算检查帧率功能
 * @li 更新maya自动解算组件：添加帧率不为25帧则不进行解算
 * @li 更新maya自动解算组件：添加maya解算标记拾取 \n
 * @li 更新ue4插件：更改abc导入预设
 * @li 更新ue4插件：将ue4 更改材质名称插件兼容静态网格体
 * @li 更新ue4插件：添加材质缓存支持选项确认并修复功能
 * @li 更新ue4插件：添加材质保存回调, 自动保存材质
 *
 *
 * @subsection updata_log_3011 版本3.0.11
 *
 * @li 更新maya自动解算组件：更新maya 解算资产替换规则
 *
 * @subsection updata_log_3013 版本3.0.13
 *
 * @li 更新maya自动解算组件：更新maya解算文件abc导出规则
 *
 * @subsection updata_log_3014 版本3.0.14
 *
 * @li 更新maya工具: 解算导出是提取路径功能
 *
 * @subsection updata_log_3015 版本3.0.15
 *
 * @li 更新maya工具: 导出时创建连续的目录
 *
 * @subsection updata_log_3016 版本3.0.16
 *
 * @li 更新maya 自动导出fbx工具 : 更新导出fbx 摄像机无法导出时导出问题
 *
 * @subsection updata_log_3017 版本3.0.17
 *
 * @li 更新maya 自动导出fbx工具 : 添加fbx导出筛选， 没有加载的不导出
 * @li 更新盘符映射改为系统连接
 *
 * @subsection updata_log_3018 版本3.0.18
 *
 * @li 更新maya 解算工具: 添加z直接解算选项
 * @warning  这个版本不能正确的设置布料缓存路径（没有选中）
 *
 * @subsection updata_log_3019 版本3.0.19
 *
 * @li 更新maya 导出fbx工具: 正确的进度条和析构节点
 *
 *
 * @subsection updata_log_3020 版本3.0.20
 *
 * @li 更新maya 解算工具: 正确的创建文件夹和选中导出物体
 * @li 更新maya 解算工具: 正确的解算起始时间
 *
 * @subsection updata_log_3021 版本3.0.21
 * @li 更新maya 解算工具: 解算拍屏为两次包装正确
 * @li 更新maya 解算工具: 添加解算管道, 将解算日志捕获为gui界面, 并隐藏cmd窗口
 *
 * @subsection updata_log_3022 版本3.0.22
 * @li 更新maya 解算工具: 添加更准确的进度条
 *
 * @subsection updata_log_3023 版本3.0.23
 * @li 更新maya 解算工具: 调整maya导出方法 先创建拍屏
 * @li 更新maya 解算工具: 调整maya打开解算文件时的问题, 先创建工作区再打开
 *
 * @subsection updata_log_3024 版本3.0.24
 * @li 更新maya 解算工具: maya致命错误中止进程
 * @li 更新maya 解算工具: 不需要设置maya qcloth post
 * @li 更新maya 解算工具: 多人场景分段解算（每人解算一次）
 *
 * @subsection updata_log_3025 版本3.0.25
 * @li 更新maya 解算工具: 更改maya cloth 解算 保存文件位置
 * @li 更新maya 解算工具: 更改maya cloth 解算 缓存位置
 *
 * @subsection updata_log_3026 版本3.0.26
 * @li 添加maya正确的插件加载行为
 * @li 添加ue 批量导入寻找材质部分
 *
 * @subsection updata_log_3027 版本3.0.27
 * @li 添加maya重新分别网格功能
 * @li 重新创建maya导出位置
 *
 * @subsection updata_log_3028 版本3.0.28
 * @li maya 解算和导出fbx自动化脚本更新： 更改maya 文件打开引用方式
 *
 * @subsection updata_log_3029 版本3.0.29
 * @li maya 解算和导出fbx自动化脚本更新： 解决maya在长时间任务时不会结束的问题
 */
