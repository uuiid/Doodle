//
// shot_render_light 路径条目类测试: 锁定 13 个输出路径条目的格式契约
//

#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/http_method/kitsu/auto_task.h>

#include <boost/test/unit_test.hpp>

#include <fstream>
#include <set>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(shot_render_light_paths)

namespace {

using namespace doodle;
using namespace doodle::http::auto_task;

entity make_episode_entity() {
  entity l_entity{};
  l_entity.name_ = "EP004";
  return l_entity;
}

entity make_shot_entity() {
  entity l_entity{};
  l_entity.name_ = "SC001";
  return l_entity;
}

shot_path_identifier make_identifier(bool in_is_simulation = false) {
  return shot_path_identifier{
      .project_code_       = "LQ",
      .episodes_           = episodes{make_episode_entity()},
      .shot_               = shot{make_shot_entity()},
      .is_simulation_task_ = in_is_simulation,
  };
}

/// project_path_ 指向不存在的目录, 保证地编预调探测稳定走 fallback 分支
shot_path_context make_context(bool in_is_simulation = false) {
  shot_path_context l_ctx{};
  l_ctx.id_                        = make_identifier(in_is_simulation);
  l_ctx.project_path_              = FSys::path{"E:/__doodle_not_exists__/Project"};
  l_ctx.episode_entity_            = make_episode_entity();
  l_ctx.scene_.scene_ue_path_      = FSys::path{"D:/sy_magic/ue_projects/LQ/EP0004/MyProject"};
  l_ctx.scene_.uproject_file_      = FSys::path{"D:/sy_magic/ue_projects/LQ/EP0004/MyProject/MyProject.uproject"};
  l_ctx.scene_.scene_extend_value_ = entity_asset_extend_value{};
  l_ctx.scene_.scene_extend_value_.pin_yin_ming_cheng_ = "ChangJing";
  return l_ctx;
}

FSys::path camera_file() { return FSys::path{"D:/render/LQ_EP004_SC001_camera_001-100.fbx"}; }

}  // namespace

BOOST_AUTO_TEST_CASE(animation_paths) {
  auto l_paths = shot_render_light_path_set{make_context(false), camera_file()};

  BOOST_CHECK_EQUAL(l_paths.clear_path_.get().generic_string(), "Content/Shot/ep0004/LQ004_sc001");
  BOOST_CHECK_EQUAL(
      l_paths.movie_pipeline_config_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/LQ_EP004_SC001_Config"
  );
  BOOST_CHECK_EQUAL(
      l_paths.level_sequence_import_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_DH/LQ_EP004_SC001_DH"
  );
  BOOST_CHECK_EQUAL(
      l_paths.create_map_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_DH/LQ_EP004_SC001_DH_LV"
  );
  BOOST_CHECK_EQUAL(l_paths.import_dir_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_DH/files/");
  BOOST_CHECK_EQUAL(l_paths.render_map_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_DH/sc001_DH");
  BOOST_CHECK_EQUAL(
      l_paths.ue_main_project_path_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/MyProject.uproject"
  );
  BOOST_CHECK_EQUAL(
      l_paths.out_file_dir_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Saved/MovieRenders/LQ_EP004_SC001"
  );
  BOOST_CHECK_EQUAL(
      l_paths.create_move_path_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Saved/MovieRenders/LQ_EP004_SC001.mp4"
  );
  BOOST_CHECK_EQUAL(
      l_paths.update_ue_path_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Content/Shot/ep0004/LQ004_sc001/Import_DH"
  );
  // 地编预调不存在时回退到主场景 map
  // 注意: conv_ue_game_path 产出的是 UE 标准的 UObject 路径 "PackagePath.ObjectName" 形式,
  // 因此结果是 "ChangJing.ChangJing" 而不是 "ChangJing", 这是 UE 加载 UObject 的正确格式
  BOOST_CHECK_EQUAL(l_paths.original_map_.get().generic_string(), "/Game/ChangJing/Map/ChangJing.ChangJing");
  BOOST_CHECK(l_paths.ground_pretreatment_sequence_.get().empty());
  BOOST_CHECK(l_paths.asset_copy().empty());
  BOOST_CHECK_EQUAL(l_paths.camera_file_path_.get().generic_string(), camera_file().generic_string());
}

BOOST_AUTO_TEST_CASE(simulation_paths) {
  auto l_paths = shot_render_light_path_set{make_context(true), camera_file()};

  BOOST_CHECK_EQUAL(
      l_paths.level_sequence_import_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_JS/LQ_EP004_SC001_JS"
  );
  BOOST_CHECK_EQUAL(
      l_paths.create_map_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_JS/LQ_EP004_SC001_JS_LV"
  );
  BOOST_CHECK_EQUAL(l_paths.import_dir_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_JS/files/");
  BOOST_CHECK_EQUAL(l_paths.render_map_.get().generic_string(), "/Game/Shot/ep0004/LQ004_sc001/Import_JS/sc001_JS");
  BOOST_CHECK_EQUAL(
      l_paths.update_ue_path_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Content/Shot/ep0004/LQ004_sc001/Import_JS"
  );
}

BOOST_AUTO_TEST_CASE(suffixed_episode_and_shot) {
  auto l_ctx = make_context(false);
  entity l_episode{};
  l_episode.name_ = "EP004G";
  entity l_shot{};
  l_shot.name_        = "SC001A";
  l_ctx.id_.episodes_ = episodes{l_episode};
  l_ctx.id_.shot_     = shot{l_shot};

  auto l_paths        = shot_render_light_path_set{l_ctx, camera_file()};

  BOOST_CHECK_EQUAL(l_paths.clear_path_.get().generic_string(), "Content/Shot/ep0004G/LQ004G_sc001A");
  BOOST_CHECK_EQUAL(l_paths.render_map_.get().generic_string(), "/Game/Shot/ep0004G/LQ004G_sc001A/Import_DH/sc001A_DH");
  BOOST_CHECK_EQUAL(
      l_paths.out_file_dir_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Saved/MovieRenders/LQ_EP004G_SC001A"
  );
}

/// 条目表必须覆盖 run_ue_assembly_arg 的全部 13 个路径字段, 且 apply_to 写到登记的那个字段
BOOST_AUTO_TEST_CASE(entry_table_covers_all_run_arg_path_fields) {
  auto l_paths   = shot_render_light_path_set{make_context(false), camera_file()};
  auto l_entries = l_paths.entries();
  BOOST_REQUIRE_EQUAL(l_entries.size(), 13);

  std::set<std::string> l_names{};
  for (auto&& l_ref : l_entries) {
    BOOST_REQUIRE(l_ref.entry_ != nullptr);
    l_names.emplace(l_ref.name_);
  }
  BOOST_CHECK_EQUAL(l_names.size(), 13);

  // 除"地编预调序列"(未命中预调时本就为空)外, 其余条目在本用例数据下都必须产出非空路径
  for (auto&& l_ref : l_entries) {
    if (l_ref.name_ == "ground_pretreatment_sequence_") continue;
    BOOST_CHECK_MESSAGE(!l_ref.entry_->get().empty(), "条目 " << l_ref.name_ << " 产出了空路径");
  }

  // 字段名与 run_ue_assembly_arg 的路径字段一一对应; 新增字段未登记时这里会失败
  const std::set<std::string> l_expected{
      "camera_file_path_",
      "ue_main_project_path_",
      "update_ue_path_",
      "clear_path_",
      "out_file_dir_",
      "original_map_",
      "render_map_",
      "create_map_",
      "import_dir_",
      "create_move_path_",
      "movie_pipeline_config_",
      "level_sequence_import_",
      "ground_pretreatment_sequence_",
  };
  for (auto&& l_name : l_expected) BOOST_CHECK_MESSAGE(l_names.contains(l_name), "条目表缺少字段: " << l_name);

  // apply_to 必须把每个条目写到它登记的那个字段
  import_and_render_ue_ns::run_ue_assembly_arg l_arg{};
  l_paths.apply_to(l_arg);
  for (auto&& l_ref : l_entries) {
    BOOST_CHECK_EQUAL((l_arg.*(l_ref.field_)).generic_string(), l_ref.entry_->get().generic_string());
  }

  // describe() 与 entries() 一致
  auto l_describe = l_paths.describe();
  BOOST_REQUIRE_EQUAL(l_describe.size(), l_entries.size());
  for (std::size_t i = 0; i < l_describe.size(); ++i) {
    BOOST_CHECK_EQUAL(l_describe[i].first, l_entries[i].name_);
    BOOST_CHECK_EQUAL(l_describe[i].second.generic_string(), l_entries[i].entry_->get().generic_string());
  }
}

/// 解算配对: cloth/hair/hair_XXX 标记与帧区间后缀都要被去掉
/// 注意 shot_file_name 用生产实际传入的 get_shots_animation_file_name(...) = "{code}_{ep}_{shot}",
/// 即 "ZM_EP127_SC025"; 剩下的 "Ch006A_rig_ch" 里只有 cloth/hair 是解算标记, rig_ch 是绑定名
BOOST_AUTO_TEST_CASE(pair_sim_keys_strips_cloth_hair_markers) {
  const std::string l_shot_file_name{"ZM_EP127_SC025"};
  const std::vector<std::string> l_stems{
      "ZM_EP127_SC025_Ch006A_rig_ch_cloth_hair_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_hair_cloth_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_cloth_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_hair_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_hair_dasbxs_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_cloth_hair_dasbxs_1001-1105",
      "ZM_EP127_SC025_Ch006A_rig_ch_1001-1105",
  };

  auto l_result = pair_sim_keys(l_stems, l_shot_file_name);
  BOOST_REQUIRE_EQUAL(l_result.size(), l_stems.size());
  for (std::size_t i = 0; i < l_result.size(); ++i) {
    BOOST_CHECK_EQUAL(l_result[i].first, "Ch006A_rig_ch");
    BOOST_CHECK_EQUAL(l_result[i].second, i);
  }
}

/// 解算配对: 没有 cloth/hair 标记时只去掉帧区间后缀
BOOST_AUTO_TEST_CASE(pair_sim_keys_without_marker) {
  const std::vector<std::string> l_stems{
      "LQ_EP004_SC001_ch_1001-1105",
      "LQ_EP004_SC001_Ch006A_rig_Low_1-2",
  };

  auto l_result = pair_sim_keys(l_stems, "LQ_EP004_SC001");
  BOOST_REQUIRE_EQUAL(l_result.size(), 2);
  BOOST_CHECK_EQUAL(l_result[0].first, "ch");
  BOOST_CHECK_EQUAL(l_result[0].second, 0);
  BOOST_CHECK_EQUAL(l_result[1].first, "Ch006A_rig_Low");
  BOOST_CHECK_EQUAL(l_result[1].second, 1);
}

// ---------------------------------------------------------------------------
// 目录扫描与 abc 剔除: 这两件事要读文件系统, 因此在临时目录里造一棵真实的输出树。
// 注意 check_files() 整体在 #ifdef NDEBUG 内, 而 test_main 只在 debug_doodle preset 构建,
// 所以这里不需要真的准备 .uasset 文件; 若将来在 Release 下构建测试, 需要补上。
// ---------------------------------------------------------------------------

namespace {

const uuid k_test_uuid{{0x01, 0x9f, 0x6a, 0x42, 0xa4, 0x9d, 0x71, 0xcd, 0x80, 0x37, 0xd8, 0x3d, 0xa7, 0xe0, 0x1f, 0x7c}
};

entity_asset_extend make_extend(std::string in_bian_hao) {
  entity_asset_extend l_extend{};
  l_extend.bian_hao_           = std::move(in_bian_hao);
  l_extend.pin_yin_ming_cheng_ = "ChangJing";
  l_extend.gui_dang_           = 1;
  l_extend.kai_shi_ji_shu_     = k_test_uuid;
  return l_extend;
}

/// 临时目录里的镜头输出树, 析构时整棵删除
class shot_output_tree {
 public:
  shot_output_tree() : l_root_(FSys::temp_directory_path() / fmt::format("doodle_srl_scan_{}", next_index())) {
    std::error_code l_ec{};
    FSys::remove_all(l_root_, l_ec);
    FSys::create_directories(fbx_dir());
    FSys::create_directories(abc_dir());
    // resolve_scene_ue_path() 经 find_ue_project_file() 从场景 map 路径逐级向上找 .uproject:
    // map 路径本身必须存在, 且某个祖先目录里要有 .uproject, 否则会抛「未找到场景 ... 的 ue 工程文件」
    const auto l_map = scene_map_path();
    FSys::create_directories(l_map.parent_path());
    touch(l_map.parent_path(), l_map.filename().string());
    touch(prj_path(), "MyProject.uproject");
  }
  ~shot_output_tree() {
    std::error_code l_ec{};
    FSys::remove_all(l_root_, l_ec);
  }
  shot_output_tree(const shot_output_tree&)            = delete;
  shot_output_tree& operator=(const shot_output_tree&) = delete;

  [[nodiscard]] FSys::path prj_path() const { return l_root_ / "Project"; }
  /// 动画输出目录 .../Shots/EP004/fbx/LQ_EP004_SC001
  [[nodiscard]] FSys::path fbx_dir() const {
    return prj_path() / "03_Workflow" / "Shots" / "EP004" / "fbx" / "LQ_EP004_SC001";
  }
  /// 解算输出目录 .../Shots/EP004/abc/LQ_EP004_SC001
  [[nodiscard]] FSys::path abc_dir() const {
    return prj_path() / "03_Workflow" / "Shots" / "EP004" / "abc" / "LQ_EP004_SC001";
  }
  /// 场景 map 路径。entity_path.h 的 get_entity_ground_ue_path()/get_entity_ground_ue_map_name()
  /// 没有 DOODLELIB_API(该头文件整个不导出), 测试 exe 链接不到, 所以这里按同样的格式复刻一份:
  ///   "BG/JD{gui_dang:02}_{kai_shi_ji_shu:02}/BG{bian_hao}/{pin}" / "Content/{pin}/Map/{pin}{_banben}.umap"
  /// asset_root_path_ 为空。若那两个函数的格式变了, 这里要同步改, 否则会抛「未找到场景 ... 的 ue 工程文件」。
  [[nodiscard]] FSys::path scene_map_path() const {
    return prj_path() / "BG" / "JD01_04" / "BGSC" / "ChangJing" / "Content" / "ChangJing" / "Map" / "ChangJing.umap";
  }

  void touch(const FSys::path& in_dir, std::string_view in_file_name) const {
    std::ofstream{(in_dir / in_file_name).string(), std::ios::binary}.close();
  }

 private:
  static std::size_t next_index() {
    static std::size_t l_counter{0};
    return ++l_counter;
  }

  FSys::path l_root_;
};

/// 角色资产行, key 为 "Ch{bian_hao}" = Ch006A
shot_render_light_asset_row make_character_row() {
  entity l_asset{};
  l_asset.entity_type_id_ = asset_type::get_character_id();
  l_asset.name_           = "Ch006A";
  return shot_render_light_asset_row{
      .asset_               = l_asset,
      .asset_extend_        = make_extend("006A"),
      .ji_shu_lie_name_     = "S01",
      .kai_shi_ji_shu_name_ = "EP004",
  };
}

/// 主场景(地编)资产行, 用于推导 ue 主工程路径
shot_render_light_asset_row make_scene_row() {
  entity l_asset{};
  l_asset.name_ = "ChangJing";
  return shot_render_light_asset_row{
      .asset_               = l_asset,
      .asset_extend_        = make_extend("SC"),
      .ji_shu_lie_name_     = "S01",
      .kai_shi_ji_shu_name_ = "EP004",
  };
}

shot_render_light_input make_scan_input(const shot_output_tree& in_tree, simulation_abc_import in_abc_import) {
  shot_render_light_input l_input{};
  l_input.project_id_              = k_test_uuid;
  l_input.shot_task_id_            = k_test_uuid;
  l_input.prj_.code_               = "LQ";
  l_input.prj_.resolution_         = "1920x1080";
  l_input.prj_.path_               = in_tree.prj_path();
  l_input.shot_task_.task_type_id_ = task_type::get_simulation_task_id();
  l_input.shot_entity_.name_       = "SC001";
  l_input.episode_entity_.name_    = "EP004";
  l_input.shot_extend_.frame_in_   = 1001;
  l_input.shot_extend_.frame_out_  = 1105;
  l_input.assets_                  = {make_character_row(), make_scene_row()};
  l_input.scene_asset_             = make_scene_row();
  l_input.abc_import_              = in_abc_import;
  return l_input;
}

using arg_t = import_and_render_ue_ns::run_ue_assembly_arg;

/// 按完整文件名查找条目 —— 同一布料的 fbx 与 abc 只有扩展名不同, 按 stem 查会混淆
const import_and_render_ue_ns::run_ue_assembly_asset_info* find_asset_file(
    const arg_t& in_arg, std::string_view in_file_name
) {
  for (auto&& l_info : in_arg.asset_infos_)
    if (l_info.shot_output_path_.filename().string() == in_file_name) return &l_info;
  return nullptr;
}

std::size_t count_abc(const arg_t& in_arg) {
  std::size_t l_ret{};
  for (auto&& l_info : in_arg.asset_infos_)
    if (l_info.shot_output_path_.extension() == ".abc") ++l_ret;
  return l_ret;
}

/// 造出「动画目录: 角色 + 相机」+「解算目录: 带 cloth 标记的 fbx 与 abc」这棵标准树
void fill_cloth_tree(const shot_output_tree& in_tree) {
  in_tree.touch(in_tree.fbx_dir(), "LQ_EP004_SC001_Ch006A_rig_ch_1001-1105.fbx");
  in_tree.touch(in_tree.fbx_dir(), "LQ_EP004_SC001_camera_1001-1105.fbx");
  in_tree.touch(in_tree.abc_dir(), "LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.fbx");
  in_tree.touch(in_tree.abc_dir(), "LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.abc");
}

}  // namespace

/// 既有形态: 解算目录的 fbx 与 abc 都进入导入列表
BOOST_AUTO_TEST_CASE(scan_simulation_with_abc_keeps_abc) {
  shot_output_tree l_tree{};
  fill_cloth_tree(l_tree);

  const auto l_ret = shot_render_light_builder{make_scan_input(l_tree, simulation_abc_import::with_abc)}.run();

  BOOST_REQUIRE_EQUAL(l_ret.asset_infos_.size(), 3);
  BOOST_CHECK_EQUAL(count_abc(l_ret), 1);
  BOOST_CHECK_EQUAL(
      l_ret.camera_file_path_.generic_string(),
      (l_tree.fbx_dir() / "LQ_EP004_SC001_camera_1001-1105.fbx").generic_string()
  );

  // 角色: 被解算配对标上 cloth, 走解算皮肤
  const auto* l_char = find_asset_file(l_ret, "LQ_EP004_SC001_Ch006A_rig_ch_1001-1105.fbx");
  BOOST_REQUIRE(l_char != nullptr);
  BOOST_CHECK(l_char->type_ == import_and_render_ue_ns::import_ue_type::char_);
  BOOST_CHECK(l_char->simulation_type_.test(0));
  BOOST_CHECK(l_char->skin_path_.generic_string().find("SK_Ch006A_cloth") != std::string::npos);
  // 布料 fbx 与 abc 都是 geo, 且都与角色共用同一个 key
  for (auto&& l_name :
       {"LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.fbx", "LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.abc"}) {
    const auto* l_geo = find_asset_file(l_ret, l_name);
    BOOST_REQUIRE(l_geo != nullptr);
    BOOST_CHECK(l_geo->type_ == import_and_render_ue_ns::import_ue_type::geo);
    BOOST_CHECK_EQUAL(l_geo->key_, "Ch006A");
    BOOST_CHECK(l_geo->skin_path_.generic_string().find("SK_Ch006A_cloth") != std::string::npos);
  }
}

/// 新形态: 只把 abc 从导入列表剔除, 其余(含角色的解算皮肤)完全不变
BOOST_AUTO_TEST_CASE(scan_simulation_without_abc_drops_abc_only) {
  shot_output_tree l_tree{};
  fill_cloth_tree(l_tree);

  const auto l_with    = shot_render_light_builder{make_scan_input(l_tree, simulation_abc_import::with_abc)}.run();
  const auto l_without = shot_render_light_builder{make_scan_input(l_tree, simulation_abc_import::without_abc)}.run();

  BOOST_REQUIRE_EQUAL(l_with.asset_infos_.size(), 3);
  BOOST_REQUIRE_EQUAL(l_without.asset_infos_.size(), 2);
  BOOST_CHECK_EQUAL(count_abc(l_without), 0);
  BOOST_CHECK_EQUAL(l_without.camera_file_path_.generic_string(), l_with.camera_file_path_.generic_string());

  // 角色条目与 cloth fbx 条目必须原样保留, 且与 with_abc 完全一致
  const auto* l_char = find_asset_file(l_without, "LQ_EP004_SC001_Ch006A_rig_ch_1001-1105.fbx");
  BOOST_REQUIRE(l_char != nullptr);
  BOOST_CHECK(l_char->type_ == import_and_render_ue_ns::import_ue_type::char_);
  BOOST_CHECK(l_char->simulation_type_.test(0));
  BOOST_CHECK(l_char->skin_path_.generic_string().find("SK_Ch006A_cloth") != std::string::npos);

  const auto* l_char_with = find_asset_file(l_with, "LQ_EP004_SC001_Ch006A_rig_ch_1001-1105.fbx");
  BOOST_REQUIRE(l_char_with != nullptr);
  BOOST_CHECK_EQUAL(l_char->skin_path_.generic_string(), l_char_with->skin_path_.generic_string());
  BOOST_CHECK(l_char->simulation_type_ == l_char_with->simulation_type_);

  // 布料 fbx 仍在
  const auto* l_cloth = find_asset_file(l_without, "LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.fbx");
  BOOST_REQUIRE(l_cloth != nullptr);
  BOOST_CHECK(l_cloth->type_ == import_and_render_ue_ns::import_ue_type::geo);
  BOOST_CHECK_EQUAL(l_cloth->key_, "Ch006A");
  // abc 已剔除
  BOOST_CHECK(find_asset_file(l_without, "LQ_EP004_SC001_Ch006A_rig_ch_cloth_1001-1105.abc") == nullptr);
  // 路径约定不变: 仍是解算的 Import_JS
  BOOST_CHECK(l_without.update_ue_path_.generic_string().find("Import_JS") != std::string::npos);
}

/// 动画任务不受影响
BOOST_AUTO_TEST_CASE(scan_animation_task_unaffected) {
  shot_output_tree l_tree{};
  fill_cloth_tree(l_tree);

  auto l_input                     = make_scan_input(l_tree, simulation_abc_import::without_abc);
  l_input.shot_task_.task_type_id_ = task_type::get_animation_id();

  const auto l_ret                 = shot_render_light_builder{std::move(l_input)}.run();

  BOOST_REQUIRE_EQUAL(l_ret.asset_infos_.size(), 1);
  BOOST_CHECK_EQUAL(count_abc(l_ret), 0);
  BOOST_CHECK(l_ret.asset_infos_[0].type_ == import_and_render_ue_ns::import_ue_type::char_);
  BOOST_CHECK(!l_ret.asset_infos_[0].simulation_type_.any());
  BOOST_CHECK(l_ret.asset_infos_[0].skin_path_.generic_string().find("SK_Ch006A_cloth") == std::string::npos);
  BOOST_CHECK(l_ret.update_ue_path_.generic_string().find("Import_DH") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
