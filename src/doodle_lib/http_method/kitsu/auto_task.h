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

#include <filesystem>
#include <optional>
#include <set>
#include <string>
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
  /// 已选定的主场景(地编)资产
  shot_render_light_asset_row scene_asset_{};
};

/// 路径条目基类: 每个输出路径条目对应一个类
class DOODLELIB_API shot_path_entry_base {
 public:
  virtual ~shot_path_entry_base()              = default;
  /// 生成该条目对应的路径
  [[nodiscard]] virtual FSys::path get() const = 0;
  [[nodiscard]] operator FSys::path() const { return get(); }
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

/// 主场景路径: 探测地编预调, 命中时使用预调总关卡, 否则使用主场景 map
class DOODLELIB_API original_map_entry : public shot_path_entry_base {
 public:
  original_map_entry(
      shot_path_identifier in_id, FSys::path in_project_path, entity in_episode_entity, FSys::path in_scene_ue_path,
      FSys::path in_uproject_stem, entity_asset_extend_value in_scene_extend_value
  );
  [[nodiscard]] FSys::path get() const override;
  /// 地编预调序列路径(未命中时为空)
  [[nodiscard]] const FSys::path& pretreatment_sequence() const { return pretreatment_sequence_; }
  /// 地编预调需要复制到场景工程的文件(未命中时为空)
  [[nodiscard]] const std::vector<import_and_render_ue_ns::file_copy_info>& asset_copy() const { return asset_copy_; }

 private:
  /// 探测地编预调目录, 命中时填充 original_map_/pretreatment_sequence_/asset_copy_
  void probe_pretreatment();

  shot_path_identifier id_;
  FSys::path project_path_;
  entity episode_entity_;
  FSys::path scene_ue_path_;
  FSys::path uproject_stem_;
  entity_asset_extend_value scene_extend_value_;

  FSys::path original_map_{};
  FSys::path pretreatment_sequence_{};
  std::vector<import_and_render_ue_ns::file_copy_info> asset_copy_{};
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

/// 生成镜头输出路径所需的全部非数据库数据(由调用方查询/推导后传入)
struct shot_path_context {
  shot_path_identifier id_{};
  FSys::path project_path_{};                       // project::path_
  entity episode_entity_{};                         // 集数实体
  FSys::path scene_ue_path_{};                      // UE 场景工程根目录
  FSys::path uproject_file_{};                      // UE 主工程文件
  entity_asset_extend_value scene_extend_value_{};  // 主场景(地编)资产扩展值
};

/// 持有全部镜头输出路径条目类
struct DOODLELIB_API shot_render_light_paths {
  shot_render_light_paths(const shot_path_context& in_ctx, FSys::path in_camera_file);

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

  /// 写入 run_ue_assembly_arg 的对应字段(保持 JSON 键与语义不变)
  void apply_to(import_and_render_ue_ns::run_ue_assembly_arg& out) const;
};

/// 由 shot_render_light 过程转换而来的类: 只做路径与资产装配计算, 不访问数据库
class DOODLELIB_API shot_render_light_builder {
 public:
  explicit shot_render_light_builder(shot_render_light_input in_input);

  /// 计算并返回 UE 装配参数(与原 shot_render_light 函数等价)
  [[nodiscard]] import_and_render_ue_ns::run_ue_assembly_arg run();

 private:
  /// 填充基础字段与镜头输出目录/文件名/帧区间后缀
  void fill_base();
  /// 主场景资产 -> ue 主工程 -> scene_ue_path_/uproject_file_
  void resolve_scene_ue_path();
  /// 扫描镜头输出目录, 填充 asset_infos_ 与相机路径
  void scan_shot_output();
  /// 扫描解算输出目录
  void scan_simulation_output();
  /// 扫描动画输出目录
  void scan_animation_output();
  /// 提取 char 类型资产的 sk key
  void build_asset_keys();
  /// 解算 abc/fbx 配对并回填 simulation_type_
  void pair_simulation_outputs();
  /// 构造 paths_ 并写入全部路径字段
  void build_paths();
  /// 资产扩展 -> skin_path_/ue_project_dir_/ban_ben_suffix_/groom_bind_path_
  void bind_asset_extends();
  /// NDEBUG 下的存在性校验与 groom binding 扫描
  void check_files();
  /// 将 skin_path_/groom_bind_path_ 转换为 /Game 路径
  void conv_asset_game_paths();
  /// 由已查询数据构造镜头标识
  [[nodiscard]] shot_path_identifier make_identifier() const;

  shot_render_light_input input_{};
  import_and_render_ue_ns::run_ue_assembly_arg ret_{};
  /// 持有全部路径条目类
  std::optional<shot_render_light_paths> paths_{};
  entity_asset_extend_value scene_extend_value_{};
  FSys::path scene_ue_path_{};
  FSys::path uproject_file_{};
  FSys::path shot_path_dir_{};
  FSys::path sim_shot_path_dir_{};
  std::set<std::string> sim_output_key_{};
  std::string shot_file_name_{};
  std::string file_end_str_{};
  bool is_simulation_task_{false};
};

import_and_render_ue_ns::run_ue_assembly_arg shot_render_light(const uuid& in_project_id, const uuid& in_shot_id);
}  // namespace doodle::http::auto_task
