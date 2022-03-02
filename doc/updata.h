#pragma once

/**
 * @page doodle_updata_log 更新日志
 * @tableofcontents
 *
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
 * @subsubsection updata_log_3126 版本3.1.26
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
 *
 *
 * @subsubsection updata_log_3147 版本3.1.47
 * @li 更改maya插件加载方式（pymel 加载插件会出现回调错误）
 * @li 更改maya自定义拍屏方案（pymel 关键字会无故传递出错）
 *
 * @subsubsection updata_log_320 版本3.2.0
 * @li 更改maya插件加载方式
 * @li 更改拍屏使用方式（使用eval回调）
 *
 * @subsubsection updata_log_321 版本3.2.1
 * @li 更新积累
 *
 * @subsubsection updata_log_322 版本3.2.2
 * @li 修复hud时间长度显示和拍屏时间长度显示
 *
 *
 * @subsubsection  updata_log_323 版本3.2.3
 * @li 解算命令重写， 更加稳定
 * @li 开启多个doodle时发生的日志冲突问题解决
 * @li maya插件稳定性提升
 *
 * @subsubsection  updata_log_324 版本3.2.4
 * @li doodle 积累更新
 *
 * @subsubsection  updata_log_325 版本3.2.5
 * @li 布料解算工具完成
 *
 * @subsubsection  updata_log_326 版本3.2.6
 * @li 修复maya安装错误
 *
 * @subsubsection  updata_log_327 版本3.2.7
 * @li 修复maya启动器检查问题
 *
 * @subsubsection  updata_log_328 版本3.2.8
 * @li 修正mayay元数据加载失败问题
 *
 * @subsubsection  updata_log_329 版本3.2.9
 * @li 更改abc按钮位置
 * @li 更改手动导出时的起始帧
 *
 * @subsubsection  updata_log_3210 版本3.2.10
 * @li 更改一些默认设置
 * @li 去除一些gui按钮
 *
 * @subsubsection  updata_log_3211 版本3.2.11
 * @li 修改名称失误
 *
 * @subsubsection  updata_log_3212 版本3.2.12
 * @li 修改包maya插件裹命令
 * @li 添加主循环task提交
 *
 * @subsubsection  updata_log_3213 版本3.2.13
 * @li 更正maya项目中获取默认项目的方式
 *
 * @subsubsection  updata_log_3214 版本3.2.14
 * @li 将创建主进程受限池, 并将线程池直接调整为硬件大小
 *
 * @subsubsection  updata_log_3215 版本3.2.15
 * @li 调整多线程空指针
 *
 * @subsubsection  updata_log_3216 版本3.2.16
 * @li 更新不加载时不导出fbx，而不是导出所有
 *
 * @subsubsection  updata_log_3217 版本3.2.17
 * @li 安装方式的小更改
 *
 * @subsubsection  updata_log_3218 版本3.2.18
 * @li maya解算补丁
 *
 *
 * @subsubsection  updata_log_3219 版本3.2.19
 * @li maya解算补丁
 *
 * @subsubsection  updata_log_3220 版本3.2.20
 * @li maya致命错误提示
 *
 * @subsubsection  updata_log_3221 版本3.2.21
 * @li maya解算时间设置bug修复
 *
 * @subsubsection updata_log_3222 版本3.2.22
 * @li 更新文件选择器
 *
 * @subsubsection updata_log_3223 版本3.2.23
 * @li 修复奇异的maya库和doodle库之间的bug
 *
 * @subsubsection updata_log_3224 版本3.2.24
 * @li 添加cam解锁
 * @li 添加cam属性解锁和主动烘培
 * @li 添加主动烘培
 * @li 更新测试
 *
 * @subsubsection updata_log_3225 版本3.2.25
 * @li preserveOutsideKeys 这个选项会导致眼睛出现问题 需要改为false
 *
 * @subsubsection updata_log_3226 版本3.2.26
 * @li 添加解锁全局着色器节点
 * @li 修改编译规则
 *
 * @subsubsection updata_log_3227 版本3.2.27
 * @li 添加解锁全局着色器节点
 *
 * @subsubsection updata_log_3228 版本3.2.28
 * @li 添加解锁全局着色器节点
 *
 *
 * @subsection updata_log_330_ 版本3.3
 *
 * @subsubsection updata_log_330 版本3.3.0
 * @li 取消服务器的使用, 使用文件型服务器
 * @li 将服务器更新为文件服务器， 并添加新项目功能和打开项目最近项目
 * @li 修正导出abc不是引用的问题
 *
 * @subsubsection updata_log_331 版本3.3.1
 * @li 更新文件选择器返回函数
 * @li maya序列化json更新
 * @li 写入配置文件时机更新
 * @li 最近添加项目设置更新
 *
 * @subsubsection updata_log_332 版本3.3.2
 * @li 添加自动项目信号和回调
 * @li 更改maya名称空间寻找方案,更加优秀
 * @li 添加基本的模型库
 * @li 项目图标加载
 * @li 鲜明的显示方式（小图标）
 * @li 方便的实体编辑
 * @li 添加了多个gui自动化的测试
 * @li 添加截图功能
 * @li 添加快捷右键菜单
 * @li 添加支持拖拽文件支持
 * @li 异步化自动保存
 * @li 优化保存数据的大小
 * @li 异步加载项目时，界面禁用修复
 * @li 调整编辑实体界面， 更加方便
 * @li 导出abc时单个物体不合成
 * @li 数据库实体细粒度保存和自动保存
 *
 * @subsubsection updata_log_333 版本3.3.3
 * @li 将更新写入文件， 保持文件时间更改一致
 * @li 添加季数搜素过滤器
 * @li 添加集数搜素过滤器
 * @li 添加镜头搜素过滤器
 * @li 添加资产树搜素过滤器
 * @li 修复maya 添加碰撞时组件缺失引起的崩溃问题
 *
 * @subsubsection updata_log_334 版本3.3.4
 * @li 添加maya碰撞体输出日志
 * @li maya插件解算初始化值更新
 *
 * @subsubsection updata_log_340 版本3.4.0
 * @li 添加项目路径存在性检查
 * @li 进一步压缩项目的大小
 * @li 添加双击打开 .doodle_db文件类型的快捷方式
 * @li 双击实体打开图标文件
 * @li 添加完成模型库基本结构
 * @li 模型库图标大小自定义缩放
 * @li 模型库快速搜素
 * @li 模型库时间排列和搜素
 * @li 模型库树形过滤器快速更新
 * @li 数据库触发器添加, 完成一致性时间更改
 * @li 保存快捷方式更新
 * @li 更加智能的资产筛选方式
 * @li 项目文件可以再多个问价夹中共存
 * @li 添加保存时的进度条和软件状态
 * @li 添加拖拽时文件自动分类
 * @li 添加拖拽文件时自动拾取缩略图
 * @li 添加右键删除动作
 * @li 添加右键打开动作
 * @li 添加右键截图动作
 *
 * @subsubsection updata_log_341 版本3.4.1
 * @li 修复命令行检查带空格的路径
 *
 * @subsubsection updata_log_342 版本3.4.2
 * @li 修复新建项目时的崩溃
 * @li 修复项目无法保存的问题
 *
 * @subsubsection updata_log_343 版本3.4.3
 * @li 修复maya 插件加载默认项目不成功的问题
 *
 * @subsubsection updata_log_344 版本3.4.4
 * @li 搜素功能 使用字符串匹配(非正则表达式只是 @b 子字符串)
 * @li 添加图标搜素过滤表达式(可以使用 @b 正则表达式)
 *
 * @subsubsection updata_log_345 版本3.4.5
 * @li 添加图片加载后台任务, 多线程加载图片
 * @li 添加删除时的gui清除
 * @li 添加ue4胶片滤镜
 * @li 加速maya插件加载速度
 *
 * @subsubsection updata_log_346 版本3.4.6
 * @li 加减选功能
 * @li 全选删除功能
 * @li 按名称排序
 * @li maya插件工具架更新
 *
 * @subsubsection updata_log_347 版本3.4.7
 * @li 更新导出csv表格美化
 * @li 更加准确的文件存在判断
 */
