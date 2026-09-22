//
// Created by TD on 24-12-30.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"
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
#include <utility>
#include <vector>

namespace doodle::http::auto_task {

/// 一行资产查询结果(由调用方查询后传入)
struct shot_render_light_asset_row {
  entity asset_{};
  entity_asset_extend asset_extend_{};
  std::string ji_shu_lie_name_{};      // 集数序列实体名
  std::string kai_shi_ji_shu_name_{};  // 开始集数实体名
};

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
/// 得到对应 char 类型资产的 stem。例(shot_file_name = "ZM_EP127_SC025_Ch006A"):
///   ZM_EP127_SC025_Ch006A_rig_ch_cloth_hair_1001-1105 -> rig_ch
///   ZM_EP127_SC025_Ch006A_rig_ch_hair_dasbxs_1001-1105 -> rig_ch
///   ZM_EP127_SC025_Ch006A_rig_ch_1001-1105 -> rig_ch
[[nodiscard]] DOODLELIB_API std::vector<std::pair<std::string, std::size_t>> pair_sim_keys(
    const std::vector<std::string>& in_stems, std::string_view in_shot_file_name
);

/// 由 shot_render_light 过程转换而来的类: 只做路径与资产装配计算, 不访问数据库
///
/// 各阶段的数据一律经参数与返回值传递, 只有 input_(输入)与 ret_(累积输出)是成员,
/// 因此阶段之间的依赖在 run() 里一眼可见, 不存在隐式的成员通信。
class DOODLELIB_API shot_render_light_builder {
 public:
  explicit shot_render_light_builder(shot_render_light_input in_input);

  /// 计算并返回 UE 装配参数(与原 shot_render_light 函数等价)
  [[nodiscard]] import_and_render_ue_ns::run_ue_assembly_arg run();

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
  /// 将 skin_path_/groom_bind_path_ 转换为 /Game 路径
  void conv_asset_game_paths();

  shot_render_light_input input_{};
  import_and_render_ue_ns::run_ue_assembly_arg ret_{};
};

import_and_render_ue_ns::run_ue_assembly_arg shot_render_light(const uuid& in_project_id, const uuid& in_shot_id);
}  // namespace doodle::http::auto_task
