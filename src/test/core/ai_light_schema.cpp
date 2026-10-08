//
// AI 灯光 schema 与持久化测试.
//
// 盯住四件事:
//   1. 迁移 30 -> 31 真的建出了六张表, 且不会对已是最新版的库重复执行 (版本门控);
//   2. 嵌套结构 (rig / metrics / intent / targets) 经 JSON 列往返后逐字节一致 ——
//      这是「DB 列是查询权威, JSON 副本只用于工件自描述」这条约定的前提,
//      一旦往返有损, 下发 UE 的载荷就和库里存的对不上了;
//   3. 「每会话至多一个定稿候选」由 accepted_session_id 上的唯一索引在库层强制, 不靠应用层自觉;
//   4. 4 小时挂起判定依赖 zoned_time 的 TEXT 字典序比较, 这个假设必须被验证.
//

#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/scope/scope_exit.hpp>
#include <boost/test/unit_test.hpp>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(ai_light_schema)

namespace {

using namespace doodle;
using namespace doodle::orm;
using namespace doodle::ai_light;

/// 枚举比较的助手。Boost.Test 要求被测类型可打印（要有 operator<<），
/// 而枚举没有；直接比 magic_enum 的名字既避开了这个要求，
/// 失败时打出来的也是可读的枚举名而不是数字。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

// 迁移 30 -> 31 建出来的六张表
constexpr std::string_view g_ai_light_tables[] = {
    "ai_light_session", "ai_light_reference", "ai_light_spec",
    "ai_light_iteration", "ai_light_candidate", "ai_light_preset",
};

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_ai_light_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

std::int64_t scalar_int(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::int64_t>(0);
}

void exec_sql(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
}

bool has_table(orm::session& in_session, std::string_view in_table) {
  return scalar_int(
             in_session,
             fmt::format("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='{}';", in_table)
         ) > 0;
}

// 把六张表全删掉, 造出「还没有 AI 灯光」的 30 版形状.
// 迁移用例必须从这个形状出发, 否则表本来就在, 断言就变成了空转.
void drop_ai_light_tables(orm::session& in_session) {
  // 先删子表, 免得外键把 DROP 挡住 (schema 里是 cascade, 但顺序删更直观)
  for (auto l_it = std::rbegin(g_ai_light_tables); l_it != std::rend(g_ai_light_tables); ++l_it)
    exec_sql(in_session, fmt::format("DROP TABLE IF EXISTS {};", *l_it));
}

// 关闭外键执行一段插入: 用例里构造的归属 id 指向并不存在的 project/task/entity,
// 而这几张表的归属列都是 NOT NULL, 关掉外键是唯一能造出最小可测数据的办法.
// 关闭状态用 guard 恢复, 避免影响同一 session 后续的插入.
template <typename Fun>
void with_fk_off(orm::session& in_session, Fun&& in_fun) {
  const auto l_fk_was_on = in_session.pragma().foreign_keys();
  in_session.pragma().foreign_keys(false);
  boost::scope::scope_exit l_guard([&in_session, l_fk_was_on]() { in_session.pragma().foreign_keys(l_fk_was_on); });
  std::forward<Fun>(in_fun)();
}

/// 造一份有内容的 rig，用来验证 JSON 列往返
lighting_rig_t make_probe_rig(const uuid& in_session_id, std::int32_t in_iteration, std::int32_t in_index) {
  lighting_rig_t l_rig{};
  l_rig.session_id_      = in_session_id;
  l_rig.iteration_       = in_iteration;
  l_rig.candidate_index_ = in_index;
  l_rig.baseline_level_  = "/Game/Test/RenderMap";
  l_rig.variant_axis_    = variant_axis_t{variant_axis_kind::key_fill_ratio, 4.0 + in_index, "光比档"};
  l_rig.provenance_      = provenance_t{"deepseek-flash", "hash_abc", {"ref_00.png", "ref_01.png"}, 12345, "stats_only"};
  l_rig.globals_.exposure_               = exposure_t{"manual", -0.3};
  l_rig.globals_.white_balance_          = white_balance_t{5600.0, 0.0};
  l_rig.globals_.environment_            = environment_t{0.6, 0.02};
  l_rig.globals_.grading_                = grading_t{1.08, 0.95, 0.01, 0.25, 0.3};

  light_t l_key{};
  l_key.id_        = "key_01";
  l_key.role_      = light_role::key;
  l_key.type_      = light_type::spot;
  l_key.anchor_    = anchor_t{anchor_space::camera, "char_Ch220B", "subject_head"};
  l_key.offset_    = offset_t{-45.0, 30.0, 3.0};
  l_key.intensity_ = intensity_t{intensity_unit::candela, 12.0};
  l_key.color_     = color_t{color_mode::temperature, 4300.0, {}, {}};
  // ies_ 是 std::string 字段，给个非空值 —— 空字符串会让 JSON 往返断言变成空转，
  // 而 `nlohmann::json{some_string}` 会静默变成单元素数组（见 common.h 的说明），
  // 那种错只有非空值才暴露得出来。
  l_key.shape_     = shape_t{25.0, "IES/spot_soft.ies", 25.0, 45.0};
  l_key.shadows_   = shadows_t{true, 0.6};
  l_key.channels_  = {0, 1, 0};
  l_key.mobility_  = light_mobility::movable;
  l_key.anim_      = {anim_key_t{1001, 12.0}, anim_key_t{1060, 18.0}};
  l_key.origin_    = light_origin::ai;

  light_t l_fill   = l_key;
  l_fill.id_       = "fill_01";
  l_fill.role_     = light_role::fill;
  l_fill.origin_   = light_origin::inherited;
  l_fill.color_    = color_t{color_mode::rgb, 5600.0, vec3_t{0.5, 0.6, 0.8}, {}};
  l_fill.mobility_ = light_mobility::stationary;

  l_rig.lights_ = {l_key, l_fill};
  l_rig.ops_    = {
      light_op_t{light_op_kind::add, "key_01", nlohmann::json::object(), "提示要求左前方冷主光"},
      light_op_t{
          light_op_kind::modify, "fill_01", nlohmann::json{{"intensity", {{"unit", "candela"}, {"value", 3.0}}}},
          "压缩 key:fill 比到 8:1"
      },
      light_op_t{light_op_kind::set_grading, {}, nlohmann::json{{"vignette", 0.25}}, "参考有明显暗角"},
  };
  return l_rig;
}

/// 造一份有内容的 spec（走内存聚合体，顺便验证表实体 ↔ 聚合体的转换）
ai_light_spec make_probe_spec(const uuid& in_session_id, std::int32_t in_revision) {
  ai_light_spec l_spec{};
  l_spec.session_id_ = in_session_id;
  l_spec.revision_   = in_revision;

  lighting_intent_t l_intent{};
  l_intent.atmosphere_    = {"压抑", "暖", "高对比"};
  l_intent.time_of_day_   = "night";
  l_intent.weather_       = "clear";
  l_intent.source_motivation_ = {"practical_lamp", "window"};
  l_intent.story_context_ = "角色在雨夜独自等待，情绪低落";
  l_intent.setting_       = "老式公寓室内，木质家具，窗外霓虹";
  l_intent.character_personality_.push_back(character_personality_t{
      "char_Ch220B", "阴郁内向",
      lighting_language_t{interval_t{6.0, 10.0}, "hard", "side_high", "cool"}
  });
  l_spec.set_intent(l_intent);

  lighting_targets_t l_targets{};
  l_targets.luma_percentiles_     = luma_percentiles_t{interval_t{0.02, 0.06}, interval_t{0.10, 0.18}, interval_t{0.55, 0.75}};
  l_targets.palette_              = {palette_entry_t{vec3_t{0.2, 0.15, 0.1}, interval_t{0.20, 0.35}}};
  l_targets.contrast_             = interval_t{0.55, 0.75};
  l_targets.white_balance_kelvin_ = interval_t{3400.0, 4200.0};
  l_targets.clip_rate_            = clip_rate_t{interval_t{0.0, 0.02}, interval_t{0.0, 0.05}};
  l_targets.grading_hints_        = grading_hints_t{"strong", true, "moderate"};
  l_targets.ownership_.lighting_  = {"luma_percentiles", "white_balance_kelvin", "clip_rate"};
  l_targets.ownership_.grading_   = {"palette", "contrast", "grading_hints"};
  l_spec.set_targets(l_targets);

  reference_stats_t l_stats{};
  l_stats.image_count_        = 4;
  l_stats.conflicts_          = {"white_balance_kelvin"};
  l_stats.retouch_detected_   = true;
  l_stats.per_dimension_confidence_["luma_percentiles"]     = 0.85;
  l_stats.per_dimension_confidence_["white_balance_kelvin"] = 0.5;
  l_spec.set_reference_stats(l_stats);

  l_spec.set_evidence({
      {"targets.contrast", {evidence_source::reference_stats, 0.85, "四张图中位数"}},
      {"intent.atmosphere", {evidence_source::prompt, 1.0, ""}},
  });
  l_spec.set_conflict_resolution(conflict_resolution_t{{{"white_balance_kelvin", nlohmann::json::array({3800.0, 4100.0})}}, "取两张冷光图的交集"});
  return l_spec;
}

}  // namespace

// 全新库: 六张表与 accepted_session_id 上的唯一索引都必须由 sync_schema 建出来.
BOOST_AUTO_TEST_CASE(fresh_db_creates_ai_light_tables) {
  app_base l_app{};
  auto l_db = temp_db("fresh");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();
    auto l_session = l_storage.create_session();

    for (const auto& l_table : g_ai_light_tables) {
      BOOST_TEST_MESSAGE(fmt::format("检查表 {} 存在", l_table));
      BOOST_TEST(has_table(l_session, l_table));
    }

    // 定稿唯一性靠 accepted_session_id 上的普通唯一索引强制.
    // 断言它确实是 UNIQUE 且**不带 WHERE 子句** ——
    // 带 WHERE 的 partial index 在这里是行不通的（SQLite 禁止其 WHERE 里出现绑定参数），
    // 而本库的 create_unique_index().where(...) 只会生成带 `?` 的比较式.
    // 这条断言同时锁住"约束真的在库里"和"它不是 partial index"两件事.
    const auto l_unique = scalar_int(
        l_session,
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND tbl_name='ai_light_candidate' "
        "AND upper(sql) LIKE 'CREATE UNIQUE INDEX%' AND upper(sql) LIKE '%ACCEPTED_SESSION_ID%';"
    );
    BOOST_TEST_MESSAGE(fmt::format("accepted_session_id 上的唯一索引数量 = {}", l_unique));
    BOOST_TEST(l_unique >= 1);

    const auto l_partial = scalar_int(
        l_session,
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND tbl_name='ai_light_candidate' "
        "AND sql IS NOT NULL AND upper(sql) LIKE '%WHERE%';"
    );
    BOOST_TEST(l_partial == 0);

    BOOST_TEST(l_session.pragma().foreign_key_check().empty());
  }

  remove_db(l_db);
}

// 30 -> 31 的副作用: 六张表被建出来.
// 生产库停在 30, 而 30 之后只有这一步 (取消会话终态不涉及 schema、也不需要数据迁移,
// 见 docs/ue_ai_lighting_plan.md §15.10), 所以终点就是 31.
BOOST_AUTO_TEST_CASE(upgrade_30_to_31_creates_ai_light_tables) {
  app_base l_app{};
  auto l_db = temp_db("from30");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    // 造 30 版形状: 还没有 AI 灯光
    drop_ai_light_tables(l_session);
    for (const auto& l_table : g_ai_light_tables) BOOST_TEST(!has_table(l_session, l_table));

    l_session.pragma().user_version(30);
    l_storage.upgrade();

    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("v30 升级后 user_version = {}", l_version));
    BOOST_TEST(l_version == 31);
    // 副作用存在, 说明这一步真的跑了
    for (const auto& l_table : g_ai_light_tables) BOOST_TEST(has_table(l_session, l_table));
    BOOST_TEST(l_session.pragma().foreign_key_check().empty());
  }

  remove_db(l_db);
}

// 已是最新版的库: 六张表被删掉也不该被重建 —— 说明这一步被版本号挡住了,
// 而不是「恰好幂等」.
//
// 起点取 31（= 最新版）而不是更旧的版本: 31 > 30 → upgrade_3_t 直接跳过,
// 版本号也停在 31 不动. 这就是门控本身.
BOOST_AUTO_TEST_CASE(upgrade_gating_skips_ai_light_when_already_latest) {
  app_base l_app{};
  auto l_db = temp_db("gating31");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    drop_ai_light_tables(l_session);
    l_session.pragma().user_version(31);
    l_storage.upgrade();

    BOOST_TEST(scalar_int(l_session, "PRAGMA user_version;") == 31);
    for (const auto& l_table : g_ai_light_tables) BOOST_TEST(!has_table(l_session, l_table));
  }

  remove_db(l_db);
}

// 嵌套结构经 JSON 列往返必须无损, 且真实列与 JSON 副本保持一致.
BOOST_AUTO_TEST_CASE(ai_light_roundtrip_through_db) {
  app_base l_app{};
  auto l_db = temp_db("roundtrip");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();
    auto l_session = l_storage.create_session();
    // 查询方法在 sqlite_database 上（生产路径走 get_sqlite_database()），
    // 测试里从 storage 手工拼一个，与 sqlite_upgrade.cpp 的做法一致。
    auto l_sql = sqlite_database{l_storage.get_strand(), l_session};

    // --- 会话: 归属 id 指向不存在的行, 关外键插入 ---
    ai_light_session l_session_row{};
    l_session_row.project_id_ = core_set::get_set().get_uuid();
    l_session_row.task_id_    = core_set::get_set().get_uuid();
    l_session_row.entity_id_  = core_set::get_set().get_uuid();
    l_session_row.title_      = "雨夜等待";
    l_session_row.prompt_text_ = "压抑、暖、高对比；角色阴郁内向；雨夜独自等待";
    l_session_row.mark_awaiting_judgement();
    with_fk_off(l_session, [&]() { insert(l_session).into<ai_light_session>().values(l_session_row)(); });
    BOOST_TEST(!l_session_row.uuid_id_.is_nil());

    // --- 参考图: 每张单独留统计 ---
    ai_light_reference l_ref{};
    l_ref.session_id_ = l_session_row.uuid_id_;
    l_ref.ordinal_    = 0;
    l_ref.file_path_  = FSys::path{"AI_Light"} / "ref" / "ref_00.png";
    l_ref.width_      = 1920;
    l_ref.height_     = 1080;
    l_ref.set_stats(reference_image_stats_t{
        "display_srgb", luma_percentiles_value_t{0.03, 0.12, 0.62}, {image_palette_entry_t{vec3_t{0.2, 0.15, 0.1}, 0.3}},
        0.68, 3900.0, clip_rate_value_t{0.005, 0.01}, 0.4
    });
    l_ref.stats_confidence_ = 0.85;
    insert(l_session).into<ai_light_reference>().values(l_ref)();

    // --- Spec ---
    auto l_spec = make_probe_spec(l_session_row.uuid_id_, 1);
    insert(l_session).into<ai_light_spec>().values(l_spec)();
    BOOST_TEST(!l_spec.uuid_id_.is_nil());

    // --- 轮次 ---
    ai_light_iteration l_iter{};
    l_iter.session_id_ = l_session_row.uuid_id_;
    l_iter.iteration_  = 1;
    l_iter.spec_id_    = l_spec.uuid_id_;
    l_iter.set_axes(iteration_axes_t{
        {axis_step_t{variant_axis_kind::key_fill_ratio, {4.0, 6.0, 8.0}, "光比"}}, "沿光比展开三档"
    });
    insert(l_session).into<ai_light_iteration>().values(l_iter)();
    BOOST_TEST(!l_iter.uuid_id_.is_nil());

    // --- 6 个候选 ---
    std::vector<lighting_rig_t> l_rigs{};
    for (std::int32_t l_i = 0; l_i != 6; ++l_i) {
      ai_light_candidate l_cand{};
      l_cand.iteration_id_ = l_iter.uuid_id_;
      l_cand.set_rig(make_probe_rig(l_session_row.uuid_id_, 1, l_i));
      // 真实列从 rig 副本同步过来（生产路径也是这样）
      l_cand.sync_from_rig();
      // 路径不是列（批 7 / D44）：候选行只留状态与数值，路径由 candidate_render_path 现算。
      l_cand.render_status_   = candidate_render_status::rendered;
      l_cand.render_ms_       = 18000 + l_i * 100;
      l_cand.light_count_     = 2;
      l_cand.set_metrics(candidate_metrics_t{
          "display_srgb", luma_percentiles_value_t{0.02, 0.09 + 0.01 * l_i, 0.60}, {},
          0.61, 3900.0, clip_rate_value_t{0.0, 0.02},
          {metric_landing_t{"luma_percentiles.p50", 0.09, interval_t{0.10, 0.18}, false, 0.05}},
          0.72, {{"luma_percentiles.p50", 0.05}}
      });
      l_cand.vlm_score_ = 0.5 + 0.05 * l_i;
      l_cand.set_vlm_delta(vlm_delta_t{"deepseek-flash", "closer", {"色温偏冷"}, "raw"});
      l_cand.is_selected_ = (l_i == 2);
      insert(l_session).into<ai_light_candidate>().values(l_cand)();
      l_rigs.push_back(l_cand.get_rig());
    }

    // --- 读回并逐项比对 ---

    // 1. 按镜头查会话
    auto l_sessions = l_sql.get_ai_light_sessions_by_entity(l_session_row.entity_id_);
    BOOST_TEST(l_sessions.size() == 1);
    BOOST_TEST(l_sessions.front().uuid_id_ == l_session_row.uuid_id_);
    BOOST_TEST(l_sessions.front().prompt_text_ == l_session_row.prompt_text_);
    BOOST_TEST(en(l_sessions.front().state_) == en(ai_light_session_state::awaiting_judgement));
    BOOST_TEST(l_sessions.front().judgement_due_at_.has_value());

    // 2. 按状态查。会话没有终态之后，这里只能拿还存在的状态来验「查不到别的东西」
    BOOST_TEST(l_sql.get_ai_light_sessions_by_state(ai_light_session_state::awaiting_judgement).size() == 1);
    BOOST_TEST(l_sql.get_ai_light_sessions_by_state(ai_light_session_state::generating).empty());

    // 3. 参考图统计往返
    auto l_refs = l_sql.get_ai_light_references(l_session_row.uuid_id_);
    BOOST_TEST(l_refs.size() == 1);
    BOOST_TEST(l_refs.front().get_stats().white_balance_kelvin_ == 3900.0);
    BOOST_TEST(l_refs.front().stats_confidence_ == 0.85);

    // 4. 当前 Spec = revision 最大的一行
    auto l_cur = l_sql.get_current_ai_light_spec(l_session_row.uuid_id_);
    BOOST_TEST(l_cur.has_value());
    BOOST_TEST(l_cur->revision_ == 1);
    BOOST_TEST(l_cur->get_intent().atmosphere_.size() == 3);
    BOOST_TEST(l_cur->get_intent().character_personality_.size() == 1);
    BOOST_TEST(l_cur->get_intent().character_personality_.front().lighting_language_.key_fill_ratio_->low_ == 6.0);
    BOOST_TEST(l_cur->get_targets().ownership_.grading_.size() == 3);
    BOOST_TEST(l_cur->get_reference_stats().image_count_ == 4);
    BOOST_TEST(l_cur->get_reference_stats().conflicts_.size() == 1);
    // 恒为 false 的常量: 不做物理反解
    BOOST_TEST(!l_cur->get_reference_stats().usable_for_physical_solve_);
    BOOST_TEST(l_cur->get_evidence().size() == 2);
    BOOST_TEST(l_cur->get_conflict_resolution().resolved_.size() == 1);

    // 5. 轮次
    auto l_iter_back = l_sql.get_ai_light_iteration(l_session_row.uuid_id_, 1);
    BOOST_TEST(l_iter_back.has_value());
    BOOST_TEST(l_iter_back->spec_id_ == l_spec.uuid_id_);
    BOOST_TEST(l_iter_back->get_axes().axes_.size() == 1);
    BOOST_TEST(l_iter_back->get_axes().axes_.front().steps_.size() == 3);

    // 6. 候选: 6 个, 按 candidate_index 升序, rig 逐字节一致
    auto l_cands = l_sql.get_ai_light_candidates(l_iter.uuid_id_);
    BOOST_TEST(l_cands.size() == 6);
    for (std::size_t l_i = 0; l_i != l_cands.size(); ++l_i) {
      BOOST_TEST(l_cands[l_i].candidate_index_ == static_cast<std::int32_t>(l_i));
      // JSON 往返无损 —— 这是「下发 UE 的载荷 == 库里存的东西」的前提。
      // 用括号而不是花括号构造 json：`nlohmann::json{rig}` 是列表初始化，
      // 会变成单元素数组 [rig]，两边都包一层数组就掩盖了真实的字段差异。
      BOOST_TEST(nlohmann::json(l_cands[l_i].get_rig()) == nlohmann::json(l_rigs[l_i]));
      // 真实列与 rig 副本一致
      BOOST_TEST(en(l_cands[l_i].variant_axis_) == en(variant_axis_kind::key_fill_ratio));
      BOOST_TEST(l_cands[l_i].session_id_ == l_session_row.uuid_id_);
      BOOST_TEST(en(l_cands[l_i].render_status_) == en(candidate_render_status::rendered));
      BOOST_TEST(l_cands[l_i].light_count_ == 2);
      BOOST_TEST(l_cands[l_i].get_metrics().landings_.size() == 1);
      BOOST_TEST(l_cands[l_i].get_vlm_delta().differences_.size() == 1);
    }

    // 7. warm start 起点 = is_selected 的候选
    auto l_selected = l_sql.get_ai_light_selected_candidate(l_session_row.uuid_id_, 1);
    BOOST_TEST(l_selected.has_value());
    BOOST_TEST(l_selected->candidate_index_ == 2);

    // 8. 跨轮查询（session_id 冗余列存在的理由）
    BOOST_TEST(l_sql.get_ai_light_candidates_by_session(l_session_row.uuid_id_).size() == 6);

    // 9. 还没有定稿候选
    BOOST_TEST(!l_sql.get_ai_light_accepted_candidate(l_session_row.uuid_id_).has_value());

    // 这个用例刻意让 session 的 project/task/entity 指向不存在的行（关外键插入），
    // 所以 foreign_key_check 必然报这 3 条。要紧的是**除了这 3 条再没有别的** ——
    // 新表之间那一圈外键（session ← reference/spec/iteration ← candidate）必须全部满足，
    // 否则说明某张表的归属列写错了或根本没建外键。
    std::set<std::string> l_bad{};
    for (const auto& l_entry : l_session.pragma().foreign_key_check())
      l_bad.insert(fmt::format("{}->{}", l_entry.table, l_entry.parent));
    for (const auto& l_msg : l_bad) BOOST_TEST_MESSAGE(fmt::format("外键违规: {}", l_msg));
    BOOST_TEST(l_bad.size() == 3);
    BOOST_TEST(l_bad.contains("ai_light_session->project"));
    BOOST_TEST(l_bad.contains("ai_light_session->task"));
    BOOST_TEST(l_bad.contains("ai_light_session->entity"));
  }

  remove_db(l_db);
}

// 「每会话至多一个定稿候选」必须由库层强制, 而不是靠应用层自觉.
BOOST_AUTO_TEST_CASE(single_accepted_candidate_is_enforced_by_db) {
  app_base l_app{};
  auto l_db = temp_db("accepted");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();
    auto l_session = l_storage.create_session();
    auto l_sql     = sqlite_database{l_storage.get_strand(), l_session};

    ai_light_session l_session_row{};
    l_session_row.project_id_ = core_set::get_set().get_uuid();
    l_session_row.task_id_    = core_set::get_set().get_uuid();
    l_session_row.entity_id_  = core_set::get_set().get_uuid();
    with_fk_off(l_session, [&]() { insert(l_session).into<ai_light_session>().values(l_session_row)(); });

    // 建真的 Spec 与轮次行：这样 iteration -> spec / iteration -> session 与
    // candidate -> iteration / candidate -> session 四条外键都能在**开着外键**的前提下满足，
    // 于是这个用例测的就是生产形态（外键全开），而不是"关掉外键随便插"。
    ai_light_spec l_spec{};
    l_spec.session_id_ = l_session_row.uuid_id_;
    l_spec.revision_   = 1;
    insert(l_session).into<ai_light_spec>().values(l_spec)();

    ai_light_iteration l_iter{};
    l_iter.session_id_ = l_session_row.uuid_id_;
    l_iter.iteration_  = 1;
    l_iter.spec_id_    = l_spec.uuid_id_;
    insert(l_session).into<ai_light_iteration>().values(l_iter)();

    auto l_make = [&](std::int32_t in_index, bool in_accepted) {
      ai_light_candidate l_cand{};
      l_cand.iteration_id_    = l_iter.uuid_id_;
      l_cand.session_id_      = l_session_row.uuid_id_;
      l_cand.candidate_index_ = in_index;
      // 定稿状态只走 mark_accepted()，它会一并维护 accepted_session_id_
      if (in_accepted) l_cand.mark_accepted();
      return l_cand;
    };

    // 两个非定稿候选: 允许（唯一索引把 NULL 视为互不相等）
    insert(l_session).into<ai_light_candidate>().values(l_make(0, false))();
    insert(l_session).into<ai_light_candidate>().values(l_make(1, false))();

    // 第一个定稿: 允许
    insert(l_session).into<ai_light_candidate>().values(l_make(2, true))();

    // 第二个定稿: 必须被库层拒绝
    BOOST_CHECK_THROW(insert(l_session).into<ai_light_candidate>().values(l_make(3, true))(), sqlite_orm_exception);

    // 定稿候选仍然只有一个
    auto l_accepted = l_sql.get_ai_light_accepted_candidate(l_session_row.uuid_id_);
    BOOST_TEST(l_accepted.has_value());
    BOOST_TEST(l_accepted->candidate_index_ == 2);
    BOOST_TEST(l_accepted->is_accepted_candidate());
    // 非定稿行不该被认成定稿
    BOOST_TEST(!l_make(9, false).is_accepted_candidate());
  }

  remove_db(l_db);
}

// 4 小时过期判定依赖 judgement_due_at 经 TEXT 列往返后仍然算得对.
// 这个假设必须被验证: 一旦格式变化（例如补零宽度不一致）或读回时解析偏了,
// 判定就会静默错掉, 表现是「会话该催了却不催」或「刚进入等判断就被判过期」.
//
// 删掉挂起扫描器之后这条判据更关键了: 过期不再有任何后台任务去物化它,
// is_judgement_overdue() 是**唯一**的过期来源, 它算错就没有第二个地方能发现.
BOOST_AUTO_TEST_CASE(overdue_judgement_survives_text_roundtrip) {
  app_base l_app{};
  auto l_db = temp_db("overdue");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();
    auto l_session = l_storage.create_session();
    auto l_sql     = sqlite_database{l_storage.get_strand(), l_session};

    const auto l_now = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    // 过期: 1 小时前就该判断了
    ai_light_session l_overdue{};
    l_overdue.project_id_ = core_set::get_set().get_uuid();
    l_overdue.task_id_    = core_set::get_set().get_uuid();
    l_overdue.entity_id_  = core_set::get_set().get_uuid();
    l_overdue.state_      = ai_light_session_state::awaiting_judgement;
    l_overdue.judgement_due_at_ = chrono::system_zoned_time{l_now.get_sys_time() - std::chrono::hours{1}};

    // 未过期: 还有 1 小时
    ai_light_session l_pending = l_overdue;
    l_pending.entity_id_        = core_set::get_set().get_uuid();
    l_pending.judgement_due_at_ = chrono::system_zoned_time{l_now.get_sys_time() + std::chrono::hours{1}};

    // 状态不对: 即使时限已过也不算过期（判定必须带上状态这一项）
    ai_light_session l_other_state = l_overdue;
    l_other_state.entity_id_        = core_set::get_set().get_uuid();
    l_other_state.state_            = ai_light_session_state::draft;

    with_fk_off(l_session, [&]() {
      insert(l_session).into<ai_light_session>().values(l_overdue)();
      insert(l_session).into<ai_light_session>().values(l_pending)();
      insert(l_session).into<ai_light_session>().values(l_other_state)();
    });

    // 全部从库里读回来再判 —— 判的是落库格式，不是内存里的对象
    auto l_back = l_sql.get_by_uuid<ai_light_session>(l_overdue.uuid_id_);
    BOOST_TEST(l_back.judgement_due_at_.has_value());
    BOOST_TEST(l_back.is_judgement_overdue(l_now));

    BOOST_TEST(!l_sql.get_by_uuid<ai_light_session>(l_pending.uuid_id_).is_judgement_overdue(l_now));
    BOOST_TEST(!l_sql.get_by_uuid<ai_light_session>(l_other_state.uuid_id_).is_judgement_overdue(l_now));

    // 过期的会话状态仍然是 awaiting_judgement —— 过期不改变状态，
    // 所以它照样能被 /judge 判断（见 ai_light_state.cpp 的回归用例）
    BOOST_TEST(en(l_back.state_) == en(ai_light_session_state::awaiting_judgement));
    BOOST_TEST(is_waiting_human(l_back.state_));
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
