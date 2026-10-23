//
// Created by TD on 24-12-30.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/ai_light/task_payload.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/task.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>

#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <doodle_core/metadata/ai_light/task_payload.h>
#include <utility>
#include <vector>

namespace doodle::http {

/// 一行资产查询结果(由调用方查询后传入)
struct shot_render_light_asset_row {
  entity asset_{};
  entity_asset_extend asset_extend_{};
  std::string ji_shu_lie_name_{};      // 集数序列实体名
  std::string kai_shi_ji_shu_name_{};  // 开始集数实体名
};

/// 解算任务 abc 输出的导入方式
enum class simulation_abc_import {
  /// abc 进入 UE 导入列表 (既有行为)
  with_abc,
  /// abc 不进入 UE 导入列表: 仍参与扫描/配对/check_files() 校验, 仅在组装前剔除 (新形态)
  without_abc,
};

DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    simulation_abc_import,

    {{simulation_abc_import::with_abc, "with_abc"}, {simulation_abc_import::without_abc, "without_abc"}}
)

/// shot_render_light 的全部输入: 由调用方查询后传入, 本模块不访问数据库
struct shot_render_light_input {
  uuid project_id_{};
  uuid shot_task_id_{};
  project prj_{};
  task shot_task_{};
  entity shot_entity_{};
  entity episode_entity_{};
  entity_shot_extend shot_extend_{};
  /// 镜头关联的全部资产, 已按 check_multiple_scene 的规则裁剪
  std::vector<shot_render_light_asset_row> assets_{};
  /// 主场景(地编)资产, 同时也是 assets_ 中的一行
  shot_render_light_asset_row scene_asset_{};
  /// 解算任务 abc 输出的导入方式; 仅当 shot_task_ 是解算任务时有意义
  simulation_abc_import abc_import_{simulation_abc_import::with_abc};
};

/// 路径条目基类: 每个输出路径条目对应一个类
///
/// 全部条目统一经 shot_path_entry_ref / shot_render_light_path_set::entries() 以基类指针访问,
/// 因此 apply_to()/describe() 只需要一份字段对应表, 而不是逐字段手写赋值。
class DOODLELIB_API shot_path_entry_base {
 public:
  virtual ~shot_path_entry_base()              = default;
  /// 生成该条目对应的路径
  [[nodiscard]] virtual FSys::path get() const = 0;
};

/// 镜头标识: 生成镜头相关路径所需的公共数据(不涉及数据库)
struct DOODLELIB_API shot_path_identifier {
  std::string project_code_{};
  episodes episodes_{};
  shot shot_{};
  bool is_simulation_task_{false};

  /// "_JS"(解算) 或 "_DH"(动画)
  [[nodiscard]] std::string import_suffix() const;
  /// "{code}{ep:03}_sc{shot:03}"
  [[nodiscard]] std::string shot_dir_name() const;
  /// "{code}_EP{ep:03}_SC{shot:03}"
  [[nodiscard]] std::string ep_sc_name() const;
  /// "/Game/Shot/ep{ep:04}/{shot_dir_name}"
  [[nodiscard]] std::string game_shot_dir() const;
};

/// 需要清理的路径 (Content/Shot/epXXXX/{code}{ep}XXX_sc{shot}XXX)
class DOODLELIB_API clear_path_entry : public shot_path_entry_base {
 public:
  explicit clear_path_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// 电影管线配置路径
class DOODLELIB_API movie_pipeline_config_entry : public shot_path_entry_base {
 public:
  explicit movie_pipeline_config_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// 渲染关卡序列(包)路径
class DOODLELIB_API level_sequence_import_entry : public shot_path_entry_base {
 public:
  explicit level_sequence_import_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// 创建的关卡(放置骨骼网格体)路径
class DOODLELIB_API create_map_entry : public shot_path_entry_base {
 public:
  explicit create_map_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// 导入的 fbx 和 abc 路径
class DOODLELIB_API import_dir_entry : public shot_path_entry_base {
 public:
  explicit import_dir_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// 渲染关卡路径
class DOODLELIB_API render_map_entry : public shot_path_entry_base {
 public:
  explicit render_map_entry(shot_path_identifier in_id);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
};

/// UE 主工程文件路径
class DOODLELIB_API ue_main_project_path_entry : public shot_path_entry_base {
 public:
  ue_main_project_path_entry(FSys::path in_scene_ue_path, FSys::path in_uproject_file);
  [[nodiscard]] FSys::path get() const override;

 private:
  FSys::path scene_ue_path_;
  FSys::path uproject_file_;
};

/// 渲染输出目录
class DOODLELIB_API out_file_dir_entry : public shot_path_entry_base {
 public:
  out_file_dir_entry(shot_path_identifier in_id, FSys::path in_scene_ue_path);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
  FSys::path scene_ue_path_;
};

/// 合成视频路径
class DOODLELIB_API create_move_path_entry : public shot_path_entry_base {
 public:
  explicit create_move_path_entry(FSys::path in_out_file_dir);
  [[nodiscard]] FSys::path get() const override;

 private:
  FSys::path out_file_dir_;
};

/// 上传的 UE 路径
class DOODLELIB_API update_ue_path_entry : public shot_path_entry_base {
 public:
  update_ue_path_entry(shot_path_identifier in_id, FSys::path in_scene_ue_path);
  [[nodiscard]] FSys::path get() const override;

 private:
  shot_path_identifier id_;
  FSys::path scene_ue_path_;
};

/// 主场景 map 路径(地编预调命中时由 shot_pretreatment_probe 给出预调总关卡)
class DOODLELIB_API original_map_entry : public shot_path_entry_base {
 public:
  explicit original_map_entry(FSys::path in_original_map);
  [[nodiscard]] FSys::path get() const override;

 private:
  FSys::path original_map_;
};

/// 地面预处理序列路径
class DOODLELIB_API ground_pretreatment_sequence_entry : public shot_path_entry_base {
 public:
  explicit ground_pretreatment_sequence_entry(FSys::path in_pretreatment_sequence);
  [[nodiscard]] FSys::path get() const override;

 private:
  FSys::path pretreatment_sequence_;
};

/// 相机文件路径
class DOODLELIB_API camera_file_path_entry : public shot_path_entry_base {
 public:
  explicit camera_file_path_entry(FSys::path in_camera_file);
  [[nodiscard]] FSys::path get() const override;

 private:
  FSys::path camera_file_;
};

/// UE 场景工程解析结果(由主场景资产推导, 不含数据库访问)
struct scene_resolution {
  FSys::path scene_ue_path_{};                      // UE 场景工程根目录
  FSys::path uproject_file_{};                      // UE 主工程文件
  entity_asset_extend_value scene_extend_value_{};  // 主场景(地编)资产扩展值
};

/// 生成镜头输出路径所需的全部数据: 查询 + 推导后传入, 路径条目类内不再查库
struct shot_path_context {
  shot_path_identifier id_{};
  FSys::path project_path_{};  // project::path_
  entity episode_entity_{};    // 集数实体(地编预调探测用)
  scene_resolution scene_{};   // 已解析的 UE 场景工程
};

/// 地编预调探测结果: 命中预调时给出预调总关卡 / 预调序列 / 需复制到场景工程的文件清单
struct DOODLELIB_API shot_pretreatment_probe {
  FSys::path original_map_{};
  FSys::path pretreatment_sequence_{};
  std::vector<import_and_render_ue_ns::file_copy_info> asset_copy_{};

  /// 执行探测(含文件系统查询): 命中则取预调结果, 否则回退主场景 map
  [[nodiscard]] static shot_pretreatment_probe run(const shot_path_context& in_ctx);
};

/// 一个路径条目与它在 run_ue_assembly_arg 中对应的字段
struct DOODLELIB_API shot_path_entry_ref {
  /// run_ue_assembly_arg 中的字段名(日志/诊断用)
  std::string_view name_;
  /// 生成该字段路径的条目类
  const shot_path_entry_base* entry_;
  /// run_ue_assembly_arg 中该路径字段的成员指针
  FSys::path import_and_render_ue_ns::run_ue_assembly_arg::* field_;
};

/// 持有全部镜头输出路径条目类
class DOODLELIB_API shot_render_light_path_set {
 public:
  shot_render_light_path_set(const shot_path_context& in_ctx, FSys::path in_camera_file);

  /// 全部 13 个条目与 run_ue_assembly_arg 字段的对应表(顺序与字段声明一致)
  [[nodiscard]] std::array<shot_path_entry_ref, 13> entries() const;
  /// (字段名, 路径) 列表, 用于日志与诊断
  [[nodiscard]] std::vector<std::pair<std::string_view, FSys::path>> describe() const;
  /// 写入 run_ue_assembly_arg 的对应字段(保持 JSON 键与语义不变)
  void apply_to(import_and_render_ue_ns::run_ue_assembly_arg& out) const;
  /// 地编预调命中时需要一并复制到场景工程的文件
  [[nodiscard]] const std::vector<import_and_render_ue_ns::file_copy_info>& asset_copy() const { return asset_copy_; }

  clear_path_entry clear_path_;
  movie_pipeline_config_entry movie_pipeline_config_;
  level_sequence_import_entry level_sequence_import_;
  create_map_entry create_map_;
  import_dir_entry import_dir_;
  render_map_entry render_map_;
  ue_main_project_path_entry ue_main_project_path_;
  out_file_dir_entry out_file_dir_;
  create_move_path_entry create_move_path_;
  update_ue_path_entry update_ue_path_;
  original_map_entry original_map_;
  ground_pretreatment_sequence_entry ground_pretreatment_sequence_;
  camera_file_path_entry camera_file_path_;

 private:
  /// 委托构造: 先做一次地编预调探测, 再初始化各条目
  ///
  /// 探测结果经参数传入(参数先于任何成员构造), 因此各条目的构造顺序无隐式依赖,
  /// 不会出现"读了尚未构造的成员"这类随成员声明顺序变化的未定义行为。
  shot_render_light_path_set(
      const shot_path_context& in_ctx, shot_pretreatment_probe in_probe, FSys::path in_camera_file
  );

  std::vector<import_and_render_ue_ns::file_copy_info> asset_copy_{};
};

/// 解算输出配对: 由各输出的文件 stem 得到 (配对 key, 下标)
///
/// 先去掉 "{shot_file_name}_" 前缀, 再去掉 _cloth / _hair / _hair_XXX 标记或帧区间后缀,
/// 得到对应 char 类型资产的 stem。例(shot_file_name = "ZM_EP127_SC025"):
///   ZM_EP127_SC025_Ch006A_rig_ch_cloth_hair_1001-1105 -> Ch006A_rig_ch
///   ZM_EP127_SC025_Ch006A_rig_ch_hair_dasbxs_1001-1105 -> Ch006A_rig_ch
///   ZM_EP127_SC025_Ch006A_rig_ch_1001-1105 -> Ch006A_rig_ch
/// 其中 "rig_ch" 是绑定名, 只有 cloth / hair / hair_XXX 才是解算标记; 角色 fbx 与解算输出
/// 必须共享同一段绑定名, 否则配不上, 角色会落到动画皮肤。
[[nodiscard]] DOODLELIB_API std::vector<std::pair<std::string, std::size_t>> pair_sim_keys(
    const std::vector<std::string>& in_stems, std::string_view in_shot_file_name
);

/// AI 灯光本轮渲染所需的**全部**装配信息(D57)
///
/// 刻意**不是** run_ue_assembly_arg: 那个类型是「服务端自己跑组装」的参数
/// (distributed_task.cpp 的 /run_ue_assembly 用), 它的 asset_infos_ / skin_path_ /
/// groom_bind_path_ / ue_asset_path_ 全部服务于「把解算输出配成 UE 资产」这件事。
/// 而 AI 灯光这条链路**一个都不用**: 工作机只读载荷的 render_map / level_sequence /
/// movie_pipeline_config / camera_file_path / frame_begin / frame_end / ue_project_path
/// (见 ai_light_render.cpp 与 UE 侧 DoodleLightingRoundRunner), 它要的是
/// 「一份能在本机打开的工程 + 在哪渲染」, 不需要知道资产是怎么配上的。
/// 于是这里只带走真正用到的字段, 顺带把扫描/配对/校验那一整套开销挡在链路之外。
struct DOODLELIB_API shot_render_light_sync_result {
  /// 渲染关卡(/Game 形式), 对应 payload.render_map_
  FSys::path render_map_{};
  /// 本镜头要渲染的 level sequence(/Game 形式), 对应 payload.level_sequence_
  FSys::path level_sequence_import_{};
  /// MRQ 画质配置(/Game 形式), 对应 payload.movie_pipeline_config_
  FSys::path movie_pipeline_config_{};
  /// 基线关卡(地编预调命中时是预调总关卡), 服务端只用来填 baseline_level_ 给候选生成
  FSys::path original_map_{};
  /// 帧区间(诊断用, D50 规定真正的区间取自序列的 playback range)
  std::int32_t begin_time_{};
  std::int32_t end_time_{};

  /// 工作机上的工程根, **相对 core_set::get_set().user_work_root_**:
  /// `{项目code}/{场景归档编号}/{uproj stem}` —— 与 task_sync(auto_task.cpp:1062)的本地缓存口径一致
  FSys::path project_dir_{};
  /// 工作机上 ue 主工程文件的**文件名**(工作机自己拼 project_dir_ 下的绝对路径)
  FSys::path uproject_file_name_{};
  /// 相机文件(/Game 形式), UE 用它摆机位
  FSys::path camera_file_path_{};

  /// 起 UE 之前要同步进本地缓存的装配素材(D57)
  std::vector<::doodle::ai_light::ai_light_assembly_file_t> assembly_files_{};
};

/// 由 shot_render_light 过程转换而来的类: 只做路径与资产装配计算, 不访问数据库
///
/// 各阶段的数据一律经参数与返回值传递, 只有 input_(输入)与 ret_(累积输出)是成员,
/// 因此阶段之间的依赖在 run() 里一眼可见, 不存在隐式的成员通信。
class DOODLELIB_API shot_render_light_builder {
 public:
  explicit shot_render_light_builder(shot_render_light_input in_input);

  /// 计算并返回 UE 装配参数(与原 shot_render_light 函数等价)
  [[nodiscard]] import_and_render_ue_ns::run_ue_assembly_arg run();

  /// AI 灯光这条链路要的装配信息(D57): 只做取数 + 纯路径拼接, **不扫描任何目录**
  ///
  /// 与 run() 的差别是少了全套扫描/配对/校验(build_asset_keys / pair_simulation_outputs /
  /// scan_shot_output / bind_asset_extends / check_files / drop_abc_assets): 那些阶段的产物
  /// (asset_infos_ / skin_path_ / groom_bind_path_ / ue_asset_path_)在 AI 灯光链路上没有任何
  /// 消费者 —— 工作机与 UE 侧只读 render_map / level_sequence / movie_pipeline_config /
  /// camera_file_path / 帧区间 / 候选, 于是这里连算都不算。
  /// 同步清单按 task_sync(auto_task.cpp:1035-1234)的声明式写法直接列出来, 不再从
  /// ue_asset_path_ 反推; 唯一保留的文件系统访问是地编预调探测的 4 次 exists
  /// (它决定 original_map_ 用主场景 map 还是预调总关卡, 是语义必需的)。
  // 注意: 类本身已带 DOODLELIB_API, 成员上再写一遍是 C2487(dll 接口类的成员不得重复声明 dll 接口)。
  [[nodiscard]] shot_render_light_sync_result run_sync_only();

 private:
  /// fill_base 的产物: 镜头输出目录 / 解算输出目录 / 输出文件名 / 帧区间后缀
  struct base_info {
    FSys::path shot_path_dir_{};
    FSys::path sim_shot_path_dir_{};
    std::string shot_file_name_{};
    std::string file_end_str_{};
  };

  /// 是否解算任务(由 input_.shot_task_ 推导, 无状态)
  [[nodiscard]] bool is_simulation_task() const;
  /// 由已查询数据构造镜头标识
  [[nodiscard]] shot_path_identifier make_identifier() const;
  /// 由已查询数据构造同步所需的最小镜头标识(不含解算/动画之分)
  [[nodiscard]] shot_path_identifier make_sync_identifier() const;
  /// 只找交付机位 fbx(文件名含 "_camera_"), 找不到按原口径抛 400
  ///
  /// 相机文件没有路径公式可算, 只能扫目录; 但这里不再顺手收集 fbx 资产输出,
  /// 所以 AI 灯光链路上只有这一次目录遍历, 不是 scan_shot_output 那一整套。
  [[nodiscard]] FSys::path find_shot_camera_file() const;

  /// 基础字段与镜头输出目录/文件名/帧区间后缀
  [[nodiscard]] base_info fill_base();
  /// 主场景资产 -> ue 主工程 -> 场景工程根目录
  [[nodiscard]] scene_resolution resolve_scene_ue_path();
  /// 扫描输出目录(解算任务先扫解算目录, 再扫动画目录), 并取相机文件
  void scan_shot_output(const base_info& in_base);
  /// 扫描解算输出目录, 返回全部解算输出的 stem
  [[nodiscard]] std::set<std::string> scan_simulation_output(const base_info& in_base);
  /// 扫描动画输出目录, 跳过 in_sim_output_key 中的解算输出
  void scan_animation_output(const base_info& in_base, const std::set<std::string>& in_sim_output_key);
  /// 提取 char 类型资产的 sk key
  void build_asset_keys(const base_info& in_base);
  /// 解算 abc/fbx 配对并回填 simulation_type_
  void pair_simulation_outputs(const base_info& in_base);
  /// 构造路径集合(局部量)并写入全部路径字段
  void build_paths(const scene_resolution& in_scene);
  /// 资产扩展 -> skin_path_/ue_project_dir_/ban_ben_suffix_/groom_bind_path_
  void bind_asset_extends(const scene_resolution& in_scene);
  /// NDEBUG 下的存在性校验与 groom binding 扫描
  void check_files();
  /// 按 task_sync 的声明式写法列出工作机同步清单(D57), 不读 ue_asset_path_
  ///
  /// 清单里的 `to_` 是**相对工程根**的相对路径(`{code}/{bian_hao_}/{uproj stem}` 之下),
  /// 因为目标根(user_work_root_)由工作机自己定; `from_` 是共享盘上的真身绝对路径。
  /// 必拷与可选不在这里区分 —— 客户端自己判断该不该覆盖(auto_task.cpp:1151 那批 option
  /// 语义), 服务端只负责把「这条链路上可能要哪些文件」说全。
  void build_assembly_files(const shot_render_light_path_set& in_paths, const scene_resolution& in_scene);
  /// 从导入列表中剔除 abc 资产(仅保留 fbx); 必须在 check_files() 之后调用
  void drop_abc_assets();
  /// 将 skin_path_/groom_bind_path_ 转换为 /Game 路径
  void conv_asset_game_paths();

  [[nodiscard]] shot_render_light_input input_{};
  import_and_render_ue_ns::run_ue_assembly_arg ret_{};
  /// 工作机装配同步清单(D57): 只在 run_sync_only() 里填充
  std::vector<::doodle::ai_light::ai_light_assembly_file_t> assembly_files_{};
};

import_and_render_ue_ns::run_ue_assembly_arg shot_render_light(
    const uuid& in_project_id, const uuid& in_shot_id,
    simulation_abc_import in_abc_import = simulation_abc_import::with_abc
);

/// AI 灯光链路专用: 只算「起 UE 要什么 + 工作机工程根 + 同步清单」, 不做扫描(D57)
///
/// 与 shot_render_light 是两个独立入口, 互不影响: 后者仍服务于 distributed_task.cpp
/// 的 /run_ue_assembly(那是真的要在服务端把解算输出配成 UE 资产并起 UE 组装)。
[[nodiscard]] DOODLELIB_API shot_render_light_sync_result shot_render_light_sync_only(
    const uuid& in_project_id, const uuid& in_shot_id,
    simulation_abc_import in_abc_import = simulation_abc_import::with_abc
);
}  // namespace doodle::http
