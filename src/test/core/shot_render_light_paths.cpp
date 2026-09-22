//
// shot_render_light 路径条目类测试: 锁定 13 个输出路径条目的格式契约
//

#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/http_method/kitsu/auto_task.h>

#include <boost/test/unit_test.hpp>

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
BOOST_AUTO_TEST_CASE(pair_sim_keys_strips_cloth_hair_markers) {
  const std::string l_shot_file_name{"ZM_EP127_SC025_Ch006A"};
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
    BOOST_CHECK_EQUAL(l_result[i].first, "rig_ch");
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

BOOST_AUTO_TEST_SUITE_END()
