//
// AI 灯光 schema 与持久化测试.
//
// 盯住四件事:
//   1. 迁移 30 -> 31 真的建出了六张表, 且不会对已是最新版的库重复执行 (版本门控);
//   2. 嵌套结构 (rig / metrics / intent / targets) 经 JSON 列往返后逐字节一致 ——
//      这是「DB 列是查询权威, JSON 副本只用于工件自描述」这条约定的前提,
//      一旦往返有损, 下发 UE 的载荷就和库里存的对不上了;
//   3. 4 小时挂起判定依赖 zoned_time 的 TEXT 字典序比较, 这个假设必须被验证.
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

// 迁移 30 -> 31 建出来的六张表。
// ai_light_spec 已从库中删除 —— 它是空表（全仓没有 insert 路径），
// 属于早期「后端用大模型做灯光推理」方案的遗留，详见 spec.h 顶部说明。
constexpr std::string_view g_ai_light_tables[] = {
    "ai_light_session", "ai_light_reference",
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

// 把六张表全删掉, 造出「还没有 AI 灯光」的形状.
// 迁移用例必须从这个形状出发, 否则表本来就在, 断言就变成了空转.
void drop_ai_light_tables(orm::session& in_session) {
  // 先删子表, 免得外键把 DROP 挡住 (schema 里是 cascade, 但顺序删更直观)
  for (auto l_it = std::rbegin(g_ai_light_tables); l_it != std::rend(g_ai_light_tables); ++l_it)
    exec_sql(in_session, fmt::format("DROP TABLE IF EXISTS {};", *l_it));
}

// 30 -> 31 这一步除了建六张表, 还给 seedance2_task_2 补了 apipy_key / result_text
// (见 sqlite_upgrade.cpp upgrade_3_t 里的 add_column).
// sync_schema() 是按**最新** schema 建的, ai_light_session_id 已经在了 —— 想造真正的 30 版形状
// 就得让它缺席. (result_text 已从 ORM 声明删除, 新库里本就不会建它, 但迁移里的 add_column 保留着,
// 所以这里仍然一并从夹具表里去掉, 让这个夹具继续模拟真正的 v30 形状.)
//
// 不处理会怎样: 迁移第一步就是 add_column 补列, 撞上已有列 → duplicate column name.
// 迁移只在 user_version < 31 时跑一次, 真实库不会出现「同一张表重复加列」, 是夹具自己造的.
//
// 不能用 create_table<seedance2::task>() 重建: 它按 ORM 声明建表, 声明里**有**
// ai_light_session_id (sqlite_database.cpp:118), 建出来还是 31 版形状, 等于没删.
// 也不能 ALTER TABLE ... DROP COLUMN: ai_light_session_id 被外键定义引用着, SQLite 直接拒绝
// (`unknown column "ai_light_session_id" in foreign key definition`).
// 所以这里手写一份**只含 30 版列**的建表语句 —— 列清单照抄 sqlite_database.cpp:99-121,
// 去掉 ai_light_session_id / result_text 两列; 外键也一并去掉, 免得引用尚未建出的 ai_light_session.
void rebuild_task_table_without_v31_columns(orm::session& in_session) {
  in_session.pragma().foreign_keys(false);
  exec_sql(in_session, "DROP TABLE IF EXISTS seedance2_task_2;");
  exec_sql(
      in_session,
      R"(CREATE TABLE seedance2_task_2 (
           id INTEGER PRIMARY KEY AUTOINCREMENT,
           uuid_id TEXT NOT NULL UNIQUE,
           user_id TEXT,
           status INTEGER,
           type INTEGER NOT NULL,
           backend INTEGER NOT NULL,
           data_request TEXT,
           text_prompt TEXT,
           preview_file TEXT,
           file_extension TEXT,
           data_response TEXT,
           ai_studio_id TEXT,
           task_id TEXT,
           created_at TEXT,
           ended_at TEXT,
           project_uuid_id TEXT,
           completion_tokens INTEGER,
           ai_generate_entity_id TEXT,
           subproject_id TEXT,
           retry_count INTEGER,
           archived INTEGER
         );)"
  );
  in_session.pragma().foreign_keys(true);
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
lighting_rig_t make_probe_rig() {
  lighting_rig_t l_rig{};
  l_rig.provenance_ = provenance_t{"deepseek-flash", "hash_abc", {"ref_00.png", "ref_01.png"}, 12345, "stats_only"};
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

// 曾经这里还有 make_probe_spec（造一份有内容的 lighting_spec_t 供 JSON 往返）。
// **已删除**：lighting_spec_t 与 lighting_intent_t 随 Spec 闸门一起删除，
// 它们的最后一个消费者是接缝 3 的 feedback 请求（那条接缝已整条移除）。

}  // namespace

// 全新库: 六张表都必须由 sync_schema 建出来.
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

    // 造 30 版形状: 还没有 AI 灯光, seedance2_task_2 也还没有迁移补的那两列
    drop_ai_light_tables(l_session);
    rebuild_task_table_without_v31_columns(l_session);
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
    rebuild_task_table_without_v31_columns(l_session);
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
    l_ref.file_path_  = FSys::path{"AI_Light"} / "ref" / "ref_00.png";
    l_ref.width_      = 1920;
    l_ref.height_     = 1080;
    l_ref.set_stats(reference_image_stats_t{
        "display_srgb", luma_percentiles_value_t{0.03, 0.12, 0.62}, {image_palette_entry_t{vec3_t{0.2, 0.15, 0.1}, 0.3}},
        0.68, 3900.0, clip_rate_value_t{0.005, 0.01}, 0.4
    });
    l_ref.stats_confidence_ = 0.85;
    insert(l_session).into<ai_light_reference>().values(l_ref)();

    // --- Spec：整条链已删除，这里不再有任何断言 ---
    // 表实体 ai_light_spec 是空表（已删），内存聚合体 lighting_spec_t 也随 Spec 闸门删除 ——
    // 它最后的消费者是接缝 3 的 feedback 请求，而那条接缝已整条移除。
    // 参考图统计的形状仍由 reference_image_stats_t 的落库往返覆盖（见上面的 ai_light_reference）。

    // --- 轮次 ---
    ai_light_iteration l_iter{};
    l_iter.session_id_ = l_session_row.uuid_id_;
    l_iter.iteration_  = 1;
    insert(l_session).into<ai_light_iteration>().values(l_iter)();
    BOOST_TEST(!l_iter.uuid_id_.is_nil());

    // --- 6 个候选 ---
    std::vector<lighting_rig_t> l_rigs{};
    for (std::int32_t l_i = 0; l_i != 6; ++l_i) {
      ai_light_candidate l_cand{};
      l_cand.iteration_id_ = l_iter.uuid_id_;
      // 候选由前端算好提交（服务端的展开器已删除），测试这里直接造 rig。
      l_cand.set_rig(make_probe_rig());
      // 路径不是列（D44）：候选行只留状态与数值，路径由 candidate_render_path 现算。
      l_cand.render_status_   = candidate_render_status::rendered;
      l_cand.set_metrics(candidate_metrics_t{
          "display_srgb", luma_percentiles_value_t{0.02, 0.09 + 0.01 * l_i, 0.60}, {},
          0.61, 3900.0, clip_rate_value_t{0.0, 0.02},
          {metric_landing_t{"luma_percentiles.p50", 0.09, interval_t{0.10, 0.18}, false, 0.05}},
          0.72, {{"luma_percentiles.p50", 0.05}}
      });
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

    // 4. 轮次
    auto l_iter_back = l_sql.get_ai_light_iteration(l_session_row.uuid_id_, 1);
    BOOST_TEST(l_iter_back.has_value());
    BOOST_TEST(l_iter_back->iteration_ == 1);

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
      BOOST_TEST(l_cands[l_i].session_id_ == l_session_row.uuid_id_);
      BOOST_TEST(en(l_cands[l_i].render_status_) == en(candidate_render_status::rendered));
      BOOST_TEST(l_cands[l_i].get_metrics().landings_.size() == 1);
    }

    // 7. warm start 起点 = is_selected 的候选
    auto l_selected = l_sql.get_ai_light_selected_candidate(l_session_row.uuid_id_, 1);
    BOOST_TEST(l_selected.has_value());
    BOOST_TEST(l_selected->candidate_index_ == 2);

    // 8. 跨轮查询（session_id 冗余列存在的理由）
    BOOST_TEST(l_sql.get_ai_light_candidates_by_session(l_session_row.uuid_id_).size() == 6);

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
