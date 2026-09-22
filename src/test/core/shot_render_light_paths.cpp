//
// shot_render_light 路径条目类测试: 锁定 13 个输出路径条目的格式契约
//

#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/http_method/kitsu/auto_task.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(shot_render_light_paths)

namespace {

using namespace doodle;
// 测试套件名与 auto_task::shot_render_light_paths 同名, 因此统一使用限定名
namespace srl = doodle::http::auto_task;

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

srl::shot_path_identifier make_identifier(bool in_is_simulation = false) {
  return srl::shot_path_identifier{
      .project_code_       = "LQ",
      .episodes_           = episodes{make_episode_entity()},
      .shot_               = shot{make_shot_entity()},
      .is_simulation_task_ = in_is_simulation,
  };
}

/// project_path_ 指向不存在的目录, 保证地编预调探测稳定走 fallback 分支
srl::shot_path_context make_context(bool in_is_simulation = false) {
  srl::shot_path_context l_ctx{};
  l_ctx.id_                 = make_identifier(in_is_simulation);
  l_ctx.project_path_       = FSys::path{"E:/__doodle_not_exists__/Project"};
  l_ctx.episode_entity_     = make_episode_entity();
  l_ctx.scene_ue_path_      = FSys::path{"D:/sy_magic/ue_projects/LQ/EP0004/MyProject"};
  l_ctx.uproject_file_      = FSys::path{"D:/sy_magic/ue_projects/LQ/EP0004/MyProject/MyProject.uproject"};
  l_ctx.scene_extend_value_ = entity_asset_extend_value{};
  l_ctx.scene_extend_value_.pin_yin_ming_cheng_ = "ChangJing";
  return l_ctx;
}

FSys::path camera_file() { return FSys::path{"D:/render/LQ_EP004_SC001_camera_001-100.fbx"}; }

}  // namespace

BOOST_AUTO_TEST_CASE(animation_paths) {
  auto l_paths = srl::shot_render_light_paths{make_context(false), camera_file()};

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
  BOOST_CHECK(l_paths.original_map_.asset_copy().empty());
  BOOST_CHECK_EQUAL(l_paths.camera_file_path_.get().generic_string(), camera_file().generic_string());
}

BOOST_AUTO_TEST_CASE(simulation_paths) {
  auto l_paths = srl::shot_render_light_paths{make_context(true), camera_file()};

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

  auto l_paths        = srl::shot_render_light_paths{l_ctx, camera_file()};

  BOOST_CHECK_EQUAL(l_paths.clear_path_.get().generic_string(), "Content/Shot/ep0004G/LQ004G_sc001A");
  BOOST_CHECK_EQUAL(l_paths.render_map_.get().generic_string(), "/Game/Shot/ep0004G/LQ004G_sc001A/Import_DH/sc001A_DH");
  BOOST_CHECK_EQUAL(
      l_paths.out_file_dir_.get().generic_string(),
      "D:/sy_magic/ue_projects/LQ/EP0004/MyProject/Saved/MovieRenders/LQ_EP004G_SC001A"
  );
}

BOOST_AUTO_TEST_SUITE_END()
