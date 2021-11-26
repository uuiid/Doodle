#pragma once

/**
 * @page doodle_updata_log 更新日志
 * @tableofcontents
 * <!--
 *
 * @section updata_log_3 版本3
 * @subsection updata_log_30 版本3.0
 *
 * @subsubsection updata_log_309 版本3.0.9
 *
 * @li 更新maya自动解算组件： 导出拍屏位置为mov文件夹
 * @li 更新maya自动解算组件： 拍屏优先使用quicktime h264解码
 * @li 更新maya自动解算组件： 保存文件位置为文件名称所在文件夹
 * @li 更新maya自动解算组件： 拍屏开始时间确定为1001开始帧
 * @li 更新ue4 批量导入功能： 导入时启用多进程导入
 *
 * @subsubsection updata_log_3010 版本3.0.10
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
 * @subsubsection updata_log_3011 版本3.0.11
 *
 * @li 更新maya自动解算组件：更新maya 解算资产替换规则
 *
 * @subsubsection updata_log_3013 版本3.0.13
 *
 * @li 更新maya自动解算组件：更新maya解算文件abc导出规则
 *
 * @subsubsection updata_log_3014 版本3.0.14
 *
 * @li 更新maya工具: 解算导出是提取路径功能
 *
 * @subsubsection updata_log_3015 版本3.0.15
 *
 * @li 更新maya工具: 导出时创建连续的目录
 *
 * @subsubsection updata_log_3016 版本3.0.16
 *
 * @li 更新maya 自动导出fbx工具 : 更新导出fbx 摄像机无法导出时导出问题
 *
 * @subsubsection updata_log_3017 版本3.0.17
 *
 * @li 更新maya 自动导出fbx工具 : 添加fbx导出筛选， 没有加载的不导出
 * @li 更新盘符映射改为系统连接
 *
 * @subsubsection updata_log_3018 版本3.0.18
 *
 * @li 更新maya 解算工具: 添加z直接解算选项
 * @warning  这个版本不能正确的设置布料缓存路径（没有选中）
 *
 * @subsubsection updata_log_3019 版本3.0.19
 *
 * @li 更新maya 导出fbx工具: 正确的进度条和析构节点
 *
 *
 * @subsubsection updata_log_3020 版本3.0.20
 *
 * @li 更新maya 解算工具: 正确的创建文件夹和选中导出物体
 * @li 更新maya 解算工具: 正确的解算起始时间
 *
 * @subsubsection updata_log_3021 版本3.0.21
 * @li 更新maya 解算工具: 解算拍屏为两次包装正确
 * @li 更新maya 解算工具: 添加解算管道, 将解算日志捕获为gui界面, 并隐藏cmd窗口
 *
 * @subsubsection updata_log_3022 版本3.0.22
 * @li 更新maya 解算工具: 添加更准确的进度条
 *
 * @subsubsection updata_log_3023 版本3.0.23
 * @li 更新maya 解算工具: 调整maya导出方法 先创建拍屏
 * @li 更新maya 解算工具: 调整maya打开解算文件时的问题, 先创建工作区再打开
 *
 * @subsubsection updata_log_3024 版本3.0.24
 * @li 更新maya 解算工具: maya致命错误中止进程
 * @li 更新maya 解算工具: 不需要设置maya qcloth post
 * @li 更新maya 解算工具: 多人场景分段解算（每人解算一次）
 *
 * @subsubsection updata_log_3025 版本3.0.25
 * @li 更新maya 解算工具: 更改maya cloth 解算 保存文件位置
 * @li 更新maya 解算工具: 更改maya cloth 解算 缓存位置
 *
 * @subsubsection updata_log_3026 版本3.0.26
 * @li 添加maya正确的插件加载行为
 * @li 添加ue 批量导入寻找材质部分
 *
 * @subsubsection updata_log_3027 版本3.0.27
 * @li 添加maya重新分别网格功能
 * @li 重新创建maya导出位置
 *
 * @subsubsection updata_log_3028 版本3.0.28
 * @li maya 解算和导出fbx自动化脚本更新： 更改maya 文件打开引用方式
 *
 * @subsubsection updata_log_3029 版本3.0.29
 * @li maya 解算和导出fbx自动化脚本更新： 解决maya在长时间任务时不会结束的问题
 *
 * @subsubsection updata_log_3031 版本3.0.31
 * @li 更新maya 解算工具: 正确的设置起始帧
 *
 * @subsubsection updata_log_3032 版本3.0.32
 * @li 更新maya 解算工具: 更改更加合理的创建缓存的方式
 * -->
 *
 * @subsection updata_log_31 版本3.1
 *
 * @subsubsection updata_log_310 版本3.1.0
 * @li 全选的界面
 * @li 可自由拖拽
 * @li 可以自定义进行配置 高度自定义化
 * @li 三款默认界面主题
 * @li 每次打开记住界面选项
 * @li 全新的长时间任务反馈
 *
 *
 * @subsubsection updata_log_311 版本3.1.1
 * @li 解算标记工具的更新
 *
 * @subsubsection updata_log_312 版本3.1.2
 * @li 添加线程池大小调整
 * @li 添加长时间任务时更加明了的名称
 * @li 创建包裹帮助函数
 * @li 添加子文件计数
 *
 * @subsubsection updata_log_313 版本3.1.3
 * @li win管道关闭修复(修复maya导出退出时下一个任务不会提交的问题)
 * @li 时间小部件更加方便使用
 * @li 更加科学的文件选择方式
 * @li 长时间任务主动清除过多过旧的任务
 * @li 调整表格布局方式, 更加方便和明了
 * @li ue4 创建镜头更加的清晰明了, 更加快捷
 * @li 服务器启动更加方便
 * @li ue4 自动导入优化
 * @li ue4 导入文件选择文件优化
 * @li 添加镜头和集数的多选功能
 * @li 添加本地运行模式, 脱离服务器可以运行
 * @li 优化文件上传
 * @li 文件列表优化
 * @li 添加上传文件自定义路径
 * @li 客户端更加科学的上传文件方式
 * @li 调整gui界面, 更加清晰明了
 * @li 添加文件时自动递增版本
 *
 * @subsubsection updata_log_314 版本3.1.4
 * @li 添加创建视频时判断序列帧功能
 * @li 自动判断ue4 文件并进行附加文件的上传
 * @li 在上传文件时自动合成序列帧, 并进行标注题头元素
 * @li 优化上传文件时的选项, 可以上传源文件和合成视频选项
 * @li 将上传文件和制作拍屏添加到多线程环境中
 * @li 添加maya插件
 * @li 添加maya插件gui
 * @li 将maya 插件更新到支持多个版本
 * @li 更改maya 插件创建布局时的选项
 * @li 添加maya插件更新解算属性gui
 * @li 添加maya插件题头版本号
 * @li 更新gui为动态库, 修复上下文不一致的问题
 * @li 添加maya 插件不正确退出的问题
 * @li 更加科学的maya元数据添加方式
 * @li 修复maya 插件客户端和服务器通信产生的死锁
 * @li 修复maya 插件客户端在没有服务器的情况下可以本地运行
 * @li 更新maya 插件安装方式
 *
 * @subsubsection updata_log_315 版本3.1.5
 * @li 将gui 配置文件分配到统一位置
 *
 * @subsubsection updata_log_316 版本3.1.6
 * @li 窗口和主窗口配置文件不一致问题
 * @li 修复解算abc问题
 * @li 修复长时间任务时的日志太多问题
 *
 * @subsubsection updata_log_317 版本3.1.7
 * @li 调整maya插件
 * @li 添加服务端注册为windows服务
 * @li 添加命令行选项
 * @li 添加maya插件回调
 * @li 更加提前的初始化， 更加清晰明了的日志记录
 * @li maya fbx相机插件导出更加智能的算法， 并且添加了排序功能
 *
 * @subsubsection updata_log_318 版本3.1.8
 * @li 修复maya安装插件问题
 *
 * @subsubsection updata_log_319 版本3.1.9
 * @li 修复配置文件写入权限不够导致失败的错误
 *
 * @subsubsection updata_log_3110 版本3.1.10
 * @li 添加maya导出时加载所有引用选项
 * @li 添加maya 相机解锁属性
 *
 * @subsubsection updata_log_3111 版本3.1.11
 * @li 将maya文件同时还复制到导出文件夹
 *
 * @subsubsection updata_log_3112 版本3.1.12
 * @li 修复配置文件文件夹获取错误
 *
 * @subsubsection updata_log_3113 版本3.1.13
 * @li 修复初始化配置界面杂乱现象
 * @li 将fbx导出和解算合成为一个界面
 * @li 更新布局方式， 添加更加方便的布局
 *
 * @subsubsection updata_log_3114 版本3.1.14
 * @li 修复没有元数据是pymel抛出的异常
 *
 * @subsubsection updata_log_3115 版本3.1.15
 * @li 添加导出fbx并解算按钮
 * @li 并将fbx导出和解算输出文件夹统一在一起
 *
 * @subsubsection updata_log_3116 版本3.1.16
 * @li 解算主动结束任务，超时设置为1小时
 * @li 解算时自动删除缓存文件
 *
 * @subsubsection updata_log_3117 版本3.1.17
 * @li 解算时自动删除缓存文件修复
 *
 * @subsubsection updata_log_3118 版本3.1.18
 * @li 解算时自动时缓存文件夹不存在时抛出异常问题
 *
 *
 * @subsubsection updata_log_3119 版本3.1.19
 * @li 修复gui初始化中错误的线程池线程数
 * @li 添加解锁后烘培相机方案
 *
 * @subsubsection updata_log_3120 版本3.1.20
 * @li maya 无论如何都要删除缓存目录
 *
 * @subsubsection updata_log_3121 版本3.1.21
 * @li maya 解算超时设置添加
 * @li 更加智能的解算超时算法
 * @li 解算超时后直接进行失败提示
 *
 *
 * @subsubsection updata_log_3122 版本3.1.22
 * @li 添加maya检查场景功能
 *  * 检查所有
 *  * 解锁法线
 *  * 检查重名
 *  * 检查大于四边面
 *  * 检查UV集
 *  * 去除大纲错误
 *  * 去除onModelChange3dc错误
 *  * 去除CgAbBlastPanelOptChangeCallback错误
 *  * 去除贼健康错误
 *
 *
 * @subsubsection updata_log_3123 版本3.1.23
 * @li 修复maya显示进度错误
 *
 * @subsubsection updata_log_3124 版本3.1.24
 * @li 更新安装程序 在安装新版本的同时，允许安装旧版
 * @li 安装时会完全删除旧版本
 *
 * @subsubsection updata_log_3125 版本3.1.25
 * @li 更新maya插件在批处理模式下正确运行
 *
 * @subsubsection updata_log_3125 版本3.1.25
 * @li 完成了基本的项目视图方案
 * @li 更新服务器
 *
 * @subsubsection updata_log_3127 版本3.1.27
 * @li 添加在无内容时初始化基本的添加工具
 *
 * @subsubsection updata_log_3128 版本3.1.28
 * @li 添加ue4 4.27插件
 *
 * @subsubsection updata_log_3129 版本3.1.29
 * @li 修复maya导出时无法保存文件导致的崩溃(未知节点导致无法保存)
 * @li 添加ue4 灯光配置文件类
 *
 * @subsubsection updata_log_3130 版本3.1.30
 * @li 更改ue插件安装后默认加载
 * @li 客户端优化镜头显示
 *
 * @subsubsection updata_log_3131 版本3.1.31
 * @li maya输出路径更改
 * @li 修复安装ue插件无法找到路径问题
 *
 * @subsubsection updata_log_3132 版本3.1.32
 * @li 修复maya 标签难以使用的默认值
 * @li maya工具导出时直接从1001 开始
 * @li 更加舒适的资产类排序
 *
 * @subsubsection updata_log_3133 版本3.1.33
 * @li 修复ue4 面板显示
 *
 * @subsubsection updata_log_3134 版本3.1.34
 * @li 更改maya 标签默认值
 * @li 添加maya拍屏工具
 *
 * @subsubsection updata_log_3135 版本3.1.35
 * @li 使maya后台拍屏和手动拍屏效果一致
 *
 *
 * @subsubsection updata_log_3136 版本3.1.36
 * @li 修正中文名称显示错误
 * @li 修正opencv无法读取格式错误
 *
 * @subsubsection updata_log_3137 版本3.1.37
 * @li 修复再复杂的场景中无法得到输出（未知问题，采用其他方案渲染）
 *
 * @subsubsection updata_log_3138 版本3.1.38
 * @li 修改批处理中maya的拍屏方法
 *
 * @subsubsection updata_log_3139 版本3.1.39
 * @li 修复拍屏命令结束帧默认值的问题
 * @li 修复拓展名称错误问题
 *
 * @subsubsection updata_log_3140 版本3.1.40
 * @li 修复添加ui节点时显示定位器种类
 * @li 添加相机名称显示过大过长时出现的问题
 *
 * @subsubsection updata_log_3141 版本3.1.41
 * @li 更改ui拍屏方式
 *
 * @subsubsection updata_log_3142 版本3.1.42
 * @li ui拍屏时强制更新并只显示模型和定位器
 *
 * @subsubsection updata_log_3143 版本3.1.43
 * @li 更改maya插件安装方式，防止错误的dll加载
 * 
 * 
 * @subsubsection updata_log_3144 版本3.1.44
 * @li 修复合成拍屏错误的大小
 * 
 * @subsubsection updata_log_3145 版本3.1.45
 * @li 修复maya插件无法正确退出的问题(应该是库函数没有正确析构)
 *
 * @subsubsection updata_log_3146 版本3.1.46
 * @li 无法保存时跳过不保存
 */
