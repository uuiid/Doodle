//
// AI 灯光：状态机与编排层测试.
//
// 与 ai_light_schema.cpp 的分工：
//   那份盯「表建得对不对、JSON 列往返有没有损、库层约束有没有生效」；
//   这份盯「状态怎么流转、事务边界对不对、接缝失败会不会留下半成品」。
//
// 本文件测的是 ai_light_orchestrator 里的编排函数，**不经过 HTTP 层**。
// 这是刻意的：编排层不依赖 http_jwt_t / session_data，所以单测不需要伪造
// JWT、不需要起 HTTP 服务器，能直接构造「一个处于某状态的会话」然后调用。
// 如果编排逻辑写在路由处理函数里，这些用例就只能靠发真实请求来测了。
//
// 关于并发模型：
//   app_base 的构造函数会注册 signal_set::async_wait 到 g_io_context()，
//   所以 g_io_context().run() 永远不会返回。因此这里**自建**一个 io_context，
//   把 sqlite_database 绑到它的 strand 上 —— run_sql() 的 dispatch 就落在这个
//   io_context 里，可以被 run() 跑完。
//
// 另外钉住「预分配的 uuid 不会被 ORM 改写」这条不变式：路由层与编排层都是先定
// uuid、再用它算文件名或被别人引用（见文件末尾那条用例）。它是「磁盘上的文件名」
// 与「库里的 uuid」同源的唯一保证，一旦被破坏，参考图会写得进取不出、候选回传必 404。
//

#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

// 协程用例支持（run_coro / run_coro_on_app / DOODLE_TEST_CASE_CORO）
#include <test/test_coro.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>
#include <magic_enum/magic_enum.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(ai_light_state)

namespace {

using namespace doodle;
using namespace doodle::orm;
using namespace doodle::ai_light;

/// 枚举比较助手：Boost.Test 需要可打印类型，而枚举没有 operator<<。
/// 比 magic_enum 的名字既绕开这个要求，失败信息也可读。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_ai_light_state_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"})
    FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

/// 用例名当临时库标签：夹具由 BOOST_FIXTURE_TEST_CASE 默认构造（在用例体之前），
/// 拿不到外面手写的 tag，用框架的当前用例名反而更准。
std::string current_case_tag() { return boost::unit_test::framework::current_test_case().p_name; }

/// 一份最小可测的 AI 灯光会话数据：会话 + Spec + 一轮 + N 个候选。
///
/// 归属 id 指向并不存在的 project/task/entity，而这几列都是 NOT NULL，
/// 所以构造期间关掉外键（与 ai_light_schema.cpp 的做法一致），析构时恢复。
///
/// 继承 coro_fixture_t 是为了让用例体直接写成协程：基类自带 io_context（ctx_）与把协程
/// 跑到底的 run()。**不要**再声明一个 ctx_，否则会遮蔽基类那个（run() 用的是基类的）。
struct fixture_t : doodle_test::coro_fixture_t {
  /// app_base 必须是第一个成员，两个理由：
  ///   1) 夹具的构造发生在用例体之前（BOOST_FIXTURE_TEST_CASE 会先默认构造夹具），而
  ///      core_set::get_set() 之类的 doodle 单例要等 app_base 构造之后才可用
  ///      （impl::self 只在 app_base 构造时赋值 —— 少了它这里会在夹具构造阶段就空指针崩掉）；
  ///   2) 它最后析构，保证库先拆掉 —— 与改造前「fixture 先析构、app_base 后析构」的次序一致。
  app_base app_{};

  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{temp_db(current_case_tag())};
  bool fk_was_on_{true};
  bool opened_{false};

  ai_light_session session_{};
  ai_light_iteration iteration_{};
  std::vector<ai_light_candidate> candidates_{};

  /// 默认构造（夹具只能这样被框架构造）：开库、关外键，并把库绑到**基类** io_context 的
  /// strand 上（而不是 storage_ 那个绑在 g_io_context() 上的）—— run_sql() 的 dispatch
  /// 就落在这里，能被 run() 跑完。
  /// 刻意不造数据：用例对形状的要求不同（状态 / 轮次数 / 有没有子行），那是 build()。
  fixture_t() {
    remove_db(path_);
    storage_.open(path_);
    storage_.upgrade();
    orm_ = storage_.create_session();
    sql_ = std::make_unique<sqlite_database>(boost::asio::make_strand(ctx_), orm_);

    fk_was_on_ = orm_.pragma().foreign_keys();
    orm_.pragma().foreign_keys(false);
    opened_ = true;
  }

  ~fixture_t() {
    if (opened_) {
      orm_.pragma().foreign_keys(fk_was_on_);
      sql_.reset();
    }
    remove_db(path_);
  }

  fixture_t(const fixture_t&)            = delete;
  fixture_t& operator=(const fixture_t&) = delete;

  /// 造数据。默认形状 = 在等人工判断的会话 + 一轮 6 个已渲染候选（多数用例要的就是它，
  /// 所以给默认值，用例体里直接写 `l_fx.build();`）。
  void build(
      ai_light_session_state in_state = ai_light_session_state::awaiting_judgement,
      ai_light_iteration_state in_iter_state = ai_light_iteration_state::awaiting_judgement,
      std::int32_t in_count = 6, bool in_with_children = true
  ) {
    session_.project_id_  = core_set::get_set().get_uuid();
    session_.task_id_     = core_set::get_set().get_uuid();
    session_.entity_id_   = core_set::get_set().get_uuid();
    session_.submitter_   = core_set::get_set().get_uuid();
    session_.title_       = "雨夜等待";
    session_.prompt_text_ = "压抑、暖、高对比；角色阴郁内向；雨夜独自等待";
    if (in_state == ai_light_session_state::awaiting_judgement)
      session_.mark_awaiting_judgement();
    else
      session_.state_ = in_state;
    session_.current_iteration_ = in_with_children ? 1 : 0;
    insert(orm_).into<ai_light_session>().values(session_)();

    if (!in_with_children) return;

    iteration_.session_id_ = session_.uuid_id_;
    iteration_.iteration_  = 1;
    iteration_.state_      = in_iter_state;
    // mark_iteration_rendered 是按 server_task_info_id 反查轮次的，所以要有个非 nil 值
    iteration_.server_task_info_id_ = core_set::get_set().get_uuid();
    insert(orm_).into<ai_light_iteration>().values(iteration_)();

    for (std::int32_t l_i = 0; l_i != in_count; ++l_i) {
      // 候选由前端算好提交（服务端的展开器已删除），测试这里直接造候选行。
      ai_light_candidate l_cand{};
      l_cand.iteration_id_    = iteration_.uuid_id_;
      l_cand.session_id_      = session_.uuid_id_;
      l_cand.candidate_index_ = l_i;
      l_cand.render_status_   = candidate_render_status::rendered;
      insert(orm_).into<ai_light_candidate>().values(l_cand)();
      candidates_.push_back(l_cand);
    }
  }

  /// 重新从库里读会话（验证落库结果，而不是内存里的副本）
  ai_light_session reload_session() { return sql_->get_by_uuid<ai_light_session>(session_.uuid_id_); }
  ai_light_iteration reload_iteration() { return sql_->get_by_uuid<ai_light_iteration>(iteration_.uuid_id_); }
  ai_light_candidate reload_candidate(std::size_t in_index) {
    return sql_->get_by_uuid<ai_light_candidate>(candidates_.at(in_index).uuid_id_);
  }
};

/// 给会话补一张参考图。
///
/// advance_session 在蒸馏 Spec 之前会校验参考图非空（决策 1：参考必有多张），
/// 所以凡是要走到 spec 分支的用例都得先有这一张 —— 反过来，缺了它就只会拿到 400。
ai_light_reference add_reference(fixture_t& in_fx) {
  ai_light_reference l_ref{};
  l_ref.session_id_       = in_fx.session_.uuid_id_;
  l_ref.file_path_        = FSys::path{"AI_Light"} / "ref" / "ref_00.png";
  l_ref.width_            = 1920;
  l_ref.height_           = 1080;
  l_ref.stats_confidence_ = 0.0;
  insert(in_fx.orm_).into<ai_light_reference>().values(l_ref)();
  return l_ref;
}

}  // namespace

// ===========================================================================
// 4 小时挂起
// ===========================================================================

DOODLE_FIXTURE_TEST_CASE_CORO(awaiting_judgement_sets_due_at_four_hours_ahead, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering);

  // 走真实的「渲染完成 → 等判断」路径，验证时限是在**这里**起算的
  // （而不是派发时起算：渲染期间美术本来就没法判断）
  const auto l_before = std::chrono::system_clock::now();
  co_await mark_iteration_rendered(
      *l_fx.sql_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, l_fx.candidates_
  );

  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(l_session.judgement_due_at_.has_value());

  const auto l_delta = l_session.judgement_due_at_->get_sys_time() - l_before;
  // 时限 = 4 小时。允许几十毫秒的执行误差，但绝不允许「没设」或「设成别的值」
  BOOST_TEST(l_delta > g_judgement_timeout - std::chrono::minutes{1});
  BOOST_TEST(l_delta < g_judgement_timeout + std::chrono::minutes{1});

  // 轮次被推到等判断
  auto l_iter = l_fx.reload_iteration();
  BOOST_TEST(en(l_iter.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_iter.render_at_.has_value());
}

// ===========================================================================
// 读模型
// ===========================================================================

BOOST_AUTO_TEST_CASE(session_detail_aggregates_all_tables) {
  // 这一条不需要协程：build_session_detail 是同步的，夹具里也没有 await 点
  fixture_t l_fx{};
  l_fx.build();

  // 补一张参考图，让聚合里每一类都有内容
  ai_light_reference l_ref{};
  l_ref.session_id_       = l_fx.session_.uuid_id_;
  l_ref.file_path_        = FSys::path{"AI_Light"} / "ref" / "ref_00.png";
  l_ref.width_            = 1920;
  l_ref.height_           = 1080;
  l_ref.stats_confidence_ = 0.0;
  insert(l_fx.orm_).into<ai_light_reference>().values(l_ref)();

  auto l_detail = build_session_detail(*l_fx.sql_, l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail.has_value());

  BOOST_TEST(l_detail->session_.uuid_id_ == l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail->references_.size() == 1);
  BOOST_TEST(l_detail->iterations_.size() == 1);
  BOOST_TEST(l_detail->candidates_.size() == static_cast<std::size_t>(6));

  // awaiting_judgement 是「在等人」，不是「已超时」—— 两者不能混为一谈
  BOOST_TEST(l_detail->is_waiting_human_);
  BOOST_TEST(!l_detail->is_judgement_overdue_);

  // 不存在的会话返回 nullopt，而不是抛异常：调用方（路由层）才好翻成 404
  BOOST_TEST(!build_session_detail(*l_fx.sql_, core_set::get_set().get_uuid()).has_value());
}

BOOST_AUTO_TEST_CASE(session_detail_reports_overdue_flag) {
  fixture_t l_fx{};
  l_fx.build();

  const auto l_past = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now() - std::chrono::hours{5}};
  l_fx.session_.judgement_due_at_ = l_past;
  update(l_fx.orm_)
      .from<ai_light_session>()
      .set(c(&ai_light_session::judgement_due_at_) = l_past)
      .where(c(&ai_light_session::uuid_id_) == l_fx.session_.uuid_id_)();

  auto l_detail = build_session_detail(*l_fx.sql_, l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail.has_value());
  BOOST_TEST(l_detail->is_waiting_human_);
  BOOST_TEST(l_detail->is_judgement_overdue_);
}

// ===========================================================================
// 接缝的闸门用例已随两条接缝一起删除。
//
// 曾经这里有两个用例：
//   * structure_feedback_rejects_unresolvable_operator —— 验「接缝 3 解析不出触发人时
//     报 400 而不是 500，且发生在联网之前」，因为那会决定 `/judge` 是「降级继续」还是
//     「整次判断失败」（§15.9）。
//   * 更早还有 generate_spec_rejects_unresolvable_operator，随 spec 接缝删除。
//
// 随「推理归前端」（角色降级 · 第二梯队）删除后，服务端不再替前端跑任何推理，
// `run_seam_inference` 在这条路径上也就没有再被判定的调用方 —— 这条闸门只留在
// 推理网关（`/inference` → ai_light.cpp）那条路径上，它的第一道判定
// 由 ai_light_dispatch.cpp 的网关用例覆盖。
//
// 「解析触发人」本身的正确性不再是 AI 灯光的议题：那是计费主体解析，与接缝无关。
// ===========================================================================

// 曾经的 generate_spec_rejects_unresolvable_operator 用例随 spec 接缝一起删除。
// 它验的是「spec 的第一道闸门（解析触发人）发生在联网之前」，而服务端已经不再蒸馏 Spec。
// （当时这条闸门还存在于 candidates / feedback 两条接缝里，那两条后来也删了 ——
// 见本文件上面的说明。）
//
// 同样删除的三条 advance_session 用例：advance_session_leaves_nothing_on_seam_failure、
// advance_session_from_draft_without_references_is_rejected、
// advance_session_redistills_from_awaiting_spec_confirm。它们的共同前提是
// 「draft / awaiting_spec_confirm 会走进 spec 分支」，而那个分支已删。
//
// 最后一条 advance_session 用例（advance_session_without_references_reports_no_step）
// 也随函数本身一起删除：它验的是「没有可推进的步骤时报 400」，而**整条链**已不存在 ——
// 状态推进现在只发生在参考图上传与 POST /session/{}/iteration，
// 后者的准入闸门由 ai_light_submit 的 IR 校验覆盖。一个恒抛 400 的空壳没有可测行为。
//
// 保留 net 结论：**任何**失败的推进都不得留下半成品行、不得丢参考图、
// 不得写 failure_reason_ —— 这条在提交与判断的用例里仍然有断言。

// ===========================================================================
// 人工闸门 1 已取消：状态准入谓词
// ===========================================================================
//
// 闸门 1（POST /session/{}/spec/confirm）连同 `awaiting_spec_confirm` 状态一起删除，
// 会话的 `draft → generating` 改由**参考图上传**推进。于是「哪些状态能提交候选」
// 从「只认 generating」放宽为「draft 或 generating」，这个真值表就是本节的用例。
//
// 为什么值得单独钉：放宽准入的**方向**必须是「多收一个 draft」，
// 绝不能顺手把 awaiting_judgement 也放进来 —— 那会让上一轮 6 个还没被判断的候选
// 变成孤儿（图已经渲染完、钱已经花掉，却再也不会有 /judge 能处理它们）。
// 一条 `if` 里两个 || 的表现差异没有别的用例能兜住。

BOOST_AUTO_TEST_CASE(can_submit_candidates_admits_draft_and_generating_only) {
  // 放行：两条合法入口
  BOOST_TEST(can_submit_candidates(ai_light_session_state::draft));
  BOOST_TEST(can_submit_candidates(ai_light_session_state::generating));

  // 拒绝：上一轮候选还挂着
  BOOST_TEST(!can_submit_candidates(ai_light_session_state::awaiting_judgement));
}

BOOST_AUTO_TEST_CASE(is_waiting_human_only_counts_the_judgement_gate) {
  // 人工闸门 1 取消后，唯一「等人工动作」的状态是 awaiting_judgement。
  // 前端待办列表读的是 is_waiting_human_ 这个标志，它由本谓词算出
  // （ai_light_orchestrator.cpp:810），所以这里同时钉住「读模型不会给出
  // 一个已经不存在的人工闸门」。
  BOOST_TEST(is_waiting_human(ai_light_session_state::awaiting_judgement));

  BOOST_TEST(!is_waiting_human(ai_light_session_state::draft));
  BOOST_TEST(!is_waiting_human(ai_light_session_state::generating));
}

// ===========================================================================
// 预设库
// ===========================================================================

BOOST_AUTO_TEST_CASE(preset_crud_roundtrip) {
  fixture_t l_fx{};
  l_fx.build();

  // 创建
  ai_light_preset l_preset{};
  l_preset.name_          = "伦勃朗";
  l_preset.tags_          = {"单灯", "侧逆", "古典"};
  l_preset.enabled_       = true;
  l_preset.embedding_dim_ = 3;
  l_preset.embedding_     = {0.1, 0.2, 0.3};
  l_preset.set_applicable(preset_applicable_t{{"interior"}, interval_t{1.0, 3.0}, true});
  insert(l_fx.orm_).into<ai_light_preset>().values(l_preset)();

  // 读回（默认只取启用的）
  auto l_all = l_fx.sql_->get_ai_light_presets(false);
  BOOST_TEST(l_all.size() == 1);
  BOOST_TEST(l_all.front().name_ == "伦勃朗");
  BOOST_TEST(l_all.front().tags_.size() == 3);
  BOOST_TEST(l_all.front().embedding_.size() == 3);
  BOOST_TEST(l_all.front().get_applicable().scene_type_.size() == 1);
  BOOST_TEST(l_all.front().get_applicable().scene_type_.front() == "interior");
  BOOST_TEST(l_all.front().get_applicable().subject_count_.has_value());

  auto l_one = l_fx.sql_->get_ai_light_preset(l_preset.uuid_id_);
  BOOST_TEST(l_one.has_value());
  BOOST_TEST(l_one->uuid_id_ == l_preset.uuid_id_);

  // 停用后，默认查询就不该再返回它
  update(l_fx.orm_)
      .from<ai_light_preset>()
      .set(c(&ai_light_preset::enabled_) = false)
      .where(c(&ai_light_preset::uuid_id_) == l_preset.uuid_id_)();
  BOOST_TEST(l_fx.sql_->get_ai_light_presets(true).empty());
  BOOST_TEST(l_fx.sql_->get_ai_light_presets(false).size() == 1);

  // 删除
  delete_from(l_fx.orm_).from<ai_light_preset>().where(c(&ai_light_preset::uuid_id_) == l_preset.uuid_id_)();
  BOOST_TEST(!l_fx.sql_->get_ai_light_preset(l_preset.uuid_id_).has_value());
  BOOST_TEST(l_fx.sql_->get_ai_light_presets(false).empty());
}

// ===========================================================================
// JSON 形状回归：optional / 时间字段必须是标量，不能是单元素数组
// ===========================================================================

BOOST_AUTO_TEST_CASE(optional_and_time_fields_serialize_as_scalars) {
  app_base l_app{};

  // 这条用例盯的是一个已经踩过的坑：`nlohmann::json{*opt}` 是**列表初始化**，
  // 会优先匹配 json 的 initializer_list 构造函数，于是
  //   j["at"] = nlohmann::json{*p.at_}   →   {"at": ["2026-09-24T...Z"]}
  // 而不是 {"at": "2026-09-24T...Z"}。写出来合法、编译通过、落库成功，
  // 只有读回来时才在 from_json 里对着数组抛 type_error.303。
  // 所以这里直接钉住**形状**，而不是只钉住值 —— 值相等是掩盖不了多包一层的。
  const auto l_now = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  ai_light_judgement_t l_j{};
  l_j.by_                    = core_set::get_set().get_uuid();
  l_j.at_                    = l_now;
  l_j.selected_candidate_id_ = core_set::get_set().get_uuid();
  l_j.feedback_text_         = "主光再低一点，色温再冷一点";

  nlohmann::json l_j_json = l_j;
  BOOST_TEST(l_j_json.is_object());
  BOOST_TEST(l_j_json.at("at").is_string());
  // 有值的时间字段必须是字符串，不是数组
  BOOST_TEST(!l_j_json.at("at").is_array());
  // 往返回来仍相等
  auto l_j_back = l_j_json.get<ai_light_judgement_t>();
  BOOST_TEST(l_j_back.at_.has_value());
  BOOST_TEST(l_j_back.feedback_text_ == "主光再低一点，色温再冷一点");

  // 无值的 optional 写 null（而不是空数组）
  ai_light_judgement_t l_empty{};
  nlohmann::json l_empty_json = l_empty;
  BOOST_TEST(l_empty_json.at("at").is_null());

  // 会话的时间 optional 同理
  ai_light_session l_session{};
  l_session.mark_awaiting_judgement();
  nlohmann::json l_s_json = l_session;
  BOOST_TEST(l_s_json.at("judgement_due_at").is_string());
  auto l_s_back = l_s_json.get<ai_light_session>();
  BOOST_TEST(l_s_back.judgement_due_at_.has_value());
  BOOST_TEST(l_s_back.is_judgement_overdue(chrono::system_zoned_time{
      chrono::current_zone(), l_now.get_sys_time() + std::chrono::hours{5}
  }));

  // std::string 字段（rig 的 ies_）也踩同一个坑，一并钉住
  shape_t l_shape{25.0, "IES/spot_soft.ies", 25.0, 45.0};
  nlohmann::json l_shape_json = l_shape;
  BOOST_TEST(l_shape_json.at("ies").is_string());
  BOOST_TEST(l_shape_json.get<shape_t>().ies_ == "IES/spot_soft.ies");

  // 详情读模型里的 optional 结构必须是对象/null，不能是数组
  ai_light_session_detail_t l_detail{};
  l_detail.session_ = l_session;
  nlohmann::json l_detail_json = l_detail;
  // 读模型要带出候选数（前端不自己写死 6）。个数由前端决定，所以它是**最近一轮的实际值**；
  // 这里没有候选，所以是 0。
  BOOST_TEST(l_detail_json.at("candidate_count_per_iteration").get<std::int32_t>() == 0);
}

// 预分配的 uuid 不会被 ORM 改写 —— 这是「文件名用 uuid」与「库里存的 uuid」同源的唯一保证。
//
// 为什么值得单测：路由层（ai_light.cpp 里先定 uuid 再算参考图路径）与编排层
// （ai_light_orchestrator.cpp 里先定轮次/候选/任务的 uuid 再互相引用）都是**先定后用**，
// 而 ORM 的 values() 只在 uuid 为 nil 时回填（orm/insert.h:93 与 :122）。哪天它改成无条件
// 回填，磁盘上的文件名与库里的 uuid 就会错位：参考图变成「写得进、再取不出」（拿库里的
// uuid 重算路径，算不到那个文件），轮次变成「回传必 404」（mark_iteration_rendered 按
// 任务 uuid 反查不到），候选的 iteration_id_ 也会指向一个不存在的轮次。
//
// 这里刻意走**延迟执行**：values() 之后对象就销毁，语句到 run_sql() 才落库，与两个生产
// 调用点的时序完全一致 —— 顺带钉住 bind_value_t 是拷贝语义（它装的是 std::any）。
DOODLE_FIXTURE_TEST_CASE_CORO(pre_assigned_uuid_survives_the_deferred_insert, fixture_t) {
  // 夹具自带 app_base（第一个成员，见 fixture_t）：core_set::get_set() 之类的 doodle 单例
  // 要等它构造之后才可用（impl::self 只在 app_base 构造时赋值），所以用例体里不再自己写
  // app_base，也不能在夹具构造阶段碰这些单例。
  l_fx.build();
  doodle::http::kitsu_ctx_t l_ctx{};
  l_ctx.root_            = FSys::path{"preset_root"};

  // 四个 uuid 一律先定下来，再用它算路径 / 被引用（与生产调用点的次序一致）
  const auto l_ref_uuid  = core_set::get_set().get_uuid();
  const auto l_task_uuid = core_set::get_set().get_uuid();
  const auto l_it_uuid   = core_set::get_set().get_uuid();
  const auto l_cand_uuid = core_set::get_set().get_uuid();

  // --- 参考图：文件名用它自己的 uuid ---
  ai_light_reference l_ref{};
  l_ref.uuid_id_        = l_ref_uuid;
  l_ref.session_id_     = l_fx.session_.uuid_id_;
  l_ref.width_          = 1920;
  l_ref.height_         = 1080;
  l_ref.file_path_      = l_ctx.get_ai_light_reference_file(l_fx.session_.uuid_id_, l_ref.uuid_id_);
  const auto l_ref_path = l_ref.file_path_;

  // --- 任务 + 轮次：轮次引用任务的 uuid ---
  server_task_info l_task{};
  l_task.uuid_id_     = l_task_uuid;
  l_task.name_        = "probe";
  l_task.status_      = server_task_info_status::submitted;
  l_task.submitter_   = l_fx.session_.submitter_;
  l_task.submit_time_ = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_task.task_id_     = l_fx.session_.task_id_;
  l_task.type_        = server_task_info_type::ai_light;

  ai_light_iteration l_it{};
  l_it.uuid_id_             = l_it_uuid;
  l_it.session_id_          = l_fx.session_.uuid_id_;
  l_it.iteration_           = 2;  // 夹具已用第 1 轮
  l_it.server_task_info_id_ = l_task.uuid_id_;  // 必须在任务 uuid 定下来之后

  // --- 候选：iteration_id_ 指向轮次；渲染路径**不是列**（D44），由三个 uuid 现算 ---
  ai_light_candidate l_cand{};
  l_cand.uuid_id_         = l_cand_uuid;
  l_cand.iteration_id_    = l_it.uuid_id_;
  l_cand.session_id_      = l_fx.session_.uuid_id_;
  l_cand.candidate_index_ = 0;
  l_cand.render_status_   = candidate_render_status::pending;
  const auto l_png_path =
      l_ctx.get_ai_light_candidate_file(l_fx.session_.uuid_id_, l_it.uuid_id_, l_cand.uuid_id_, false);
  const auto l_exr_path =
      l_ctx.get_ai_light_candidate_file(l_fx.session_.uuid_id_, l_it.uuid_id_, l_cand.uuid_id_, true);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_fx.orm_).into<ai_light_reference>().values(l_ref));
  l_sqls.emplace_back(insert(l_fx.orm_).into<server_task_info>().values(l_task));
  l_sqls.emplace_back(insert(l_fx.orm_).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(insert(l_fx.orm_).into<ai_light_candidate>().values(l_cand));

  // 1) 回填只补 nil：预分配的一个都不许动
  BOOST_TEST(l_ref.uuid_id_ == l_ref_uuid);
  BOOST_TEST(l_task.uuid_id_ == l_task_uuid);
  BOOST_TEST(l_it.uuid_id_ == l_it_uuid);
  BOOST_TEST(l_cand.uuid_id_ == l_cand_uuid);

  co_await l_fx.sql_->run_sql(std::move(l_sqls));

  // 2) 参考图：库里的 uuid 必须能重算出磁盘上那个文件名（夹具不造参考图，所以只有这一行）
  const auto l_refs = l_fx.sql_->get_ai_light_references(l_fx.session_.uuid_id_);
  BOOST_REQUIRE(l_refs.size() == 1);
  BOOST_TEST(l_refs.front().uuid_id_ == l_ref_uuid);
  BOOST_TEST(l_refs.front().file_path_ == l_ref_path);
  BOOST_TEST(
      l_ctx.get_ai_light_reference_file(l_fx.session_.uuid_id_, l_refs.front().uuid_id_) == l_refs.front().file_path_
  );

  // 3) 轮次：按任务 uuid 能反查回来（mark_iteration_rendered 的入口）
  const auto l_it_back = l_fx.sql_->get_ai_light_iteration_by_task(l_task_uuid);
  BOOST_REQUIRE(l_it_back.has_value());
  BOOST_TEST(l_it_back->uuid_id_ == l_it_uuid);
  BOOST_TEST(l_it_back->server_task_info_id_ == l_task_uuid);

  // 4) 候选：iteration_id_ 就是预分配的轮次 uuid（「候选属于本轮」的校验靠它）
  const auto l_cands = l_fx.sql_->get_ai_light_candidates(l_it_uuid);
  BOOST_REQUIRE(l_cands.size() == 1);
  BOOST_TEST(l_cands.front().uuid_id_ == l_cand_uuid);
  BOOST_TEST(l_cands.front().iteration_id_ == l_it_uuid);
  // 产物路径不落库（D44），但候选行自带三个 uuid，所以路径可复现：
  // 轮次目录以轮次 uuid 命名、文件以候选 uuid 命名，与 insert 之前算出来的完全一致。
  BOOST_TEST(l_png_path.parent_path().filename().generic_string() == fmt::to_string(l_it_uuid));
  BOOST_TEST(l_png_path.filename().generic_string() == fmt::to_string(l_cand_uuid) + ".png");
  BOOST_TEST(l_exr_path.filename().generic_string() == fmt::to_string(l_cand_uuid) + ".exr");
  BOOST_TEST(
      l_ctx.get_ai_light_candidate_file(
          l_cands.front().session_id_, l_cands.front().iteration_id_, l_cands.front().uuid_id_, false
      ) == l_png_path
  );
  BOOST_TEST(
      l_ctx.get_ai_light_candidate_file(
          l_cands.front().session_id_, l_cands.front().iteration_id_, l_cands.front().uuid_id_, true
      ) == l_exr_path
  );
}

BOOST_AUTO_TEST_SUITE_END()
