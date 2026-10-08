//
// AI 灯光：工作机渲染回传（派发链路的下半段）.
//
// 派发链路是「服务端下发 → 工作机跑 UE → 工作机回传」三段，这份盯的是第三段落地后
// 服务端怎么把结果写回库，也就是这两件事：
//
//   1. parse_round_render_report —— 回传报文解析。分界是**严格 vs 宽松**（D26）：
//      严格字段（`candidates` 数组本身、每条的 `candidate_id` / `status`）坏了整份拒掉，
//      抛 bad_gateway；宽松字段（`candidate_index` / `render_ms` / `light_count` / `metrics`）
//      坏了只丢那一个字段，已经渲出来的 6 张图不能因为一个诊断数字不合规就作废。
//
//   2. apply_render_report —— 两条出口（D28）：
//      至少一张 rendered → mark_iteration_rendered（轮次进 awaiting_judgement、会话
//      mark_awaiting_judgement、耗时/灯数/指标落候选行）；
//      否则，或者报文带了 error → mark_iteration_failed（**只**写会话 failure_reason_，
//      轮次状态与候选行都不动 —— 失败不是终态，美术再点一次 /iterate 就重派一轮）。
//
//   3. 归属校验 —— 任务号查不到轮次 404；候选不属本轮 400。后者是安全边界：一个坏回传
//      不能改到别的会话的行上。
//
// 为什么专门钉一条「路径不变」：回传报文里**不带**产物路径（D27），服务端只能用自己库里
// 那份（派发时就写好的）。这里断言渲染完成后候选行的路径一字不变，就是那条规则的回归钉子。
//
// 为什么不经过 HTTP 层：路由只是 get_json + parse + apply 三步，语义全在这两个函数里；
// 单测直接调它们能拿到确定的错误码。JWT / 任务类型这些代理层的东西只有真机能验，
// 不在份内（ai_light.cpp 的路由覆盖不到）。
//

#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

// 协程用例支持（run_coro / DOODLE_TEST_CASE_CORO / DOODLE_FIXTURE_TEST_CASE_CORO）
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

BOOST_AUTO_TEST_SUITE(ai_light_dispatch)

namespace {

using namespace doodle;
using namespace doodle::orm;
using namespace doodle::ai_light;

/// 枚举比较助手：Boost.Test 需要可打印类型，而枚举没有 operator<<。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_ai_light_dispatch_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"})
    FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

/// 用例名当临时库标签（夹具由框架在用例体之前默认构造，拿不到外面写的 tag）
std::string current_case_tag() { return boost::unit_test::framework::current_test_case().p_name; }

/// 「会话 + Spec + 一轮 + N 个候选」。归属 id 指向并不存在的 project/task/entity，
/// 而这几列都是 NOT NULL，所以构造期间关掉外键（与 ai_light_state.cpp 一致），析构时恢复。
///
/// 与 ai_light_state.cpp 的那份是同一个形状，但只留回传用例要的部分：
/// 候选行带 render_png_path_（「路径只能来自库」那条用例靠它）。
struct fixture_t : doodle_test::coro_fixture_t {
  /// app_base 必须是第一个成员：core_set::get_set() 之类的单例要等它构造之后才可用，
  /// 而夹具的构造发生在用例体之前。它最后析构，保证库先拆掉。
  app_base app_{};

  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{temp_db(current_case_tag())};
  bool fk_was_on_{true};
  bool opened_{false};

  ai_light_session session_{};
  ai_light_spec spec_{};
  ai_light_iteration iteration_{};
  std::vector<ai_light_candidate> candidates_{};

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

  /// 默认形状 = 正在渲染的一轮，N 个候选。回传用例几乎都要「轮次还没判断」这个前提，
  /// 所以默认给 rendering；要测「判断过的轮次拒绝回填」的用例显式传 judged。
  void build(
      ai_light_session_state in_state            = ai_light_session_state::generating,
      ai_light_iteration_state in_iter_state     = ai_light_iteration_state::rendering,
      std::int32_t in_count                      = g_candidate_count
  ) {
    session_.project_id_  = core_set::get_set().get_uuid();
    session_.task_id_     = core_set::get_set().get_uuid();
    session_.entity_id_   = core_set::get_set().get_uuid();
    session_.submitter_   = core_set::get_set().get_uuid();
    session_.title_       = "雨夜等待";
    session_.prompt_text_ = "压抑、暖、高对比；角色阴郁内向；雨夜独自等待";
    session_.state_       = in_state;
    session_.current_iteration_ = 1;
    insert(orm_).into<ai_light_session>().values(session_)();

    spec_.session_id_ = session_.uuid_id_;
    spec_.revision_   = 1;
    insert(orm_).into<ai_light_spec>().values(spec_)();

    iteration_.session_id_ = session_.uuid_id_;
    iteration_.iteration_  = 1;
    iteration_.spec_id_    = spec_.uuid_id_;
    iteration_.state_      = in_iter_state;
    // 回传是按 server_task_info_id 反查轮次的，所以要有个非 nil 值
    iteration_.server_task_info_id_ = core_set::get_set().get_uuid();
    iteration_.set_axes(iteration_axes_t{
        {axis_step_t{variant_axis_kind::key_fill_ratio, {4.0, 6.0, 8.0}, "光比"}}, "沿光比展开三档"
    });
    insert(orm_).into<ai_light_iteration>().values(iteration_)();

    for (std::int32_t l_i = 0; l_i != in_count; ++l_i) {
      ai_light_candidate l_cand{};
      l_cand.iteration_id_    = iteration_.uuid_id_;
      l_cand.session_id_      = session_.uuid_id_;
      l_cand.candidate_index_ = l_i;
      l_cand.variant_axis_    = variant_axis_kind::key_fill_ratio;
      l_cand.variant_value_   = 4.0 + 2.0 * l_i;
      l_cand.variant_label_   = fmt::format("光比 {:.1f}", l_cand.variant_value_);
      // 派发时是 pending，渲染完成后服务端回填成 rendered
      l_cand.render_status_   = candidate_render_status::pending;
      l_cand.light_count_     = 0;
      l_cand.render_png_path_ = FSys::path{"AI_Light"} / "it_001" / fmt::format("cand_{:02}.png", l_i);
      insert(orm_).into<ai_light_candidate>().values(l_cand)();
      candidates_.push_back(l_cand);
    }
  }

  ai_light_session reload_session() { return sql_->get_by_uuid<ai_light_session>(session_.uuid_id_); }
  ai_light_iteration reload_iteration() { return sql_->get_by_uuid<ai_light_iteration>(iteration_.uuid_id_); }
  ai_light_candidate reload_candidate(std::size_t in_index) {
    return sql_->get_by_uuid<ai_light_candidate>(candidates_.at(in_index).uuid_id_);
  }
};

// ---------------------------------------------------------------------------
// 报文组装助手：与工作机 ai_light_render.cpp 写出来的形状一致
// ---------------------------------------------------------------------------

nlohmann::json candidate_json(const ai_light_candidate& in_cand, const char* in_status = "rendered") {
  nlohmann::json l_j{};
  l_j["candidate_index"] = in_cand.candidate_index_;
  l_j["candidate_id"]    = in_cand.uuid_id_;
  l_j["status"]          = in_status;
  l_j["render_ms"]       = 12345;
  l_j["light_count"]     = 3;
  l_j["metrics"]         = nlohmann::json{{"space", "display_srgb"}, {"contrast", 0.42}};
  return l_j;
}

nlohmann::json ok_report(const fixture_t& in_fx, std::size_t in_count) {
  nlohmann::json l_j{};
  l_j["candidates"] = nlohmann::json::array();
  for (std::size_t l_i = 0; l_i != in_count && l_i != in_fx.candidates_.size(); ++l_i)
    l_j["candidates"].push_back(candidate_json(in_fx.candidates_.at(l_i)));
  return l_j;
}

/// 断言某个 awaitable 以指定状态码抛 doodle_error。协程里允许 try/catch 包 co_await，
/// 所以直接在这里展开（模板参数是 awaitable 的返回类型，void 时用 std::monostate 不方便，
/// 统一让调用方传 lambda 更省事 —— 见下面的 catch_status 助手）。
template <typename Fn>
boost::beast::http::status catch_status(Fn&& in_fn) {
  try {
    in_fn();
  } catch (const doodle_error& in_err) {
    return in_err.code_status_();
  }
  return boost::beast::http::status::ok;
}

}  // namespace

// ===========================================================================
// 第一段：报文解析（严格 / 宽松的分界）
// ===========================================================================

BOOST_FIXTURE_TEST_CASE(parse_full_round_keeps_every_field, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  const auto l_report = parse_round_render_report(ok_report(l_fx, 2));

  BOOST_TEST(l_report.error_.empty());
  BOOST_TEST(l_report.candidates_.size() == 2u);
  BOOST_TEST(l_report.candidates_.at(0).candidate_index_ == 0);
  BOOST_TEST(l_report.candidates_.at(0).candidate_id_ == l_fx.candidates_.at(0).uuid_id_);
  BOOST_TEST(en(l_report.candidates_.at(0).status_) == en(candidate_render_status::rendered));
  BOOST_TEST(l_report.candidates_.at(0).render_ms_ == 12345);
  BOOST_TEST(l_report.candidates_.at(0).light_count_ == 3);
  BOOST_TEST(l_report.candidates_.at(0).metrics_.contrast_ == 0.42);
}

BOOST_FIXTURE_TEST_CASE(parse_defaults_missing_loose_fields, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // 只给两个严格字段：其余全部缺失。缺失 = 取默认，不是拒。
  nlohmann::json l_json{};
  l_json["candidates"] = nlohmann::json::array({
      nlohmann::json{
          {"candidate_id", l_fx.candidates_.at(0).uuid_id_},
          {"status", "rendered"},
      },
  });

  const auto l_report = parse_round_render_report(l_json);

  BOOST_TEST(l_report.candidates_.size() == 1u);
  // candidate_index 缺失时按下标推：工作机的回传顺序就是派发顺序
  BOOST_TEST(l_report.candidates_.at(0).candidate_index_ == 0);
  BOOST_TEST(l_report.candidates_.at(0).render_ms_ == 0);
  BOOST_TEST(l_report.candidates_.at(0).light_count_ == 0);
  // metrics 缺失 → 默认值（space 有默认 "display_srgb"），不是空对象
  BOOST_TEST(l_report.candidates_.at(0).metrics_.space_ == std::string{"display_srgb"});
  BOOST_TEST(en(l_report.candidates_.at(0).status_) == en(candidate_render_status::rendered));
}

BOOST_FIXTURE_TEST_CASE(parse_requires_candidates_array, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // candidates 本身是严格字段：没有它整份回传没有意义（有 error 才是合法的「整轮失败」报文）
  const auto l_status = catch_status([] { (void)parse_round_render_report(nlohmann::json{{"error", "UE 起不来"}}); });
  BOOST_TEST(l_status == boost::beast::http::status::bad_gateway);
}

BOOST_FIXTURE_TEST_CASE(parse_rejects_unknown_status, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // "succeeded" 不是本工程的取值（candidate_render_status = pending/rendered/failed）。
  // 这条同时钉住「enum 读取走 magic_enum::enum_cast，不认识就抛」——
  // 而不是 nlohmann 那套「悄悄退化成第一个枚举量」（见候选状态机的血泪项 29）。
  auto l_json = ok_report(l_fx, 1);
  l_json["candidates"].at(0)["status"] = "succeeded";
  const auto l_status = catch_status([&] { (void)parse_round_render_report(l_json); });
  BOOST_TEST(l_status == boost::beast::http::status::bad_gateway);
}

BOOST_FIXTURE_TEST_CASE(parse_rejects_bad_candidate_uuid, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  auto l_json = ok_report(l_fx, 1);
  l_json["candidates"].at(0)["candidate_id"] = "not-a-uuid";
  const auto l_status = catch_status([&] { (void)parse_round_render_report(l_json); });
  BOOST_TEST(l_status == boost::beast::http::status::bad_gateway);
}

BOOST_FIXTURE_TEST_CASE(parse_rejects_non_object_candidate, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  nlohmann::json l_json{};
  l_json["candidates"] = nlohmann::json::array({42});
  const auto l_status = catch_status([&] { (void)parse_round_render_report(l_json); });
  BOOST_TEST(l_status == boost::beast::http::status::bad_gateway);
}

BOOST_FIXTURE_TEST_CASE(parse_keeps_round_error_text, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // 工作机在 UE 起不来时回传的就是这种：error 有内容、candidates 可能为空数组。
  // 这是**合法**报文（不是坏报文），所以不能抛；error_ 原样留着给会话 failure_reason_ 用。
  nlohmann::json l_json{};
  l_json["error"]      = "UE 启动失败：uproject 不存在";
  l_json["candidates"] = nlohmann::json::array();

  const auto l_report = parse_round_render_report(l_json);
  BOOST_TEST(l_report.error_ == std::string{"UE 启动失败：uproject 不存在"});
  BOOST_TEST(l_report.candidates_.empty());
}

BOOST_FIXTURE_TEST_CASE(parse_drops_broken_metrics_only, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // metrics 是诊断数据（给美术看落点、给 VLM 评分用）。形状错就丢这一个字段，
  // 不能让 6 张已经渲出来的图因为一个诊断对象作废 —— 这就是宽松字段存在的理由。
  auto l_json = ok_report(l_fx, 1);
  l_json["candidates"].at(0)["metrics"] = "oops";

  const auto l_report = parse_round_render_report(l_json);
  BOOST_TEST(l_report.candidates_.size() == 1u);
  BOOST_TEST(en(l_report.candidates_.at(0).status_) == en(candidate_render_status::rendered));
  BOOST_TEST(l_report.candidates_.at(0).metrics_.contrast_ == 0.0);
  BOOST_TEST(l_report.candidates_.at(0).render_ms_ == 12345);
}

// ===========================================================================
// 第二段：落库。至少一张 rendered → 轮次与会话进待判断
// ===========================================================================

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_advances_iteration_and_session, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(ok_report(l_fx, 2)));

  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();

  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_it.render_at_.has_value());
  BOOST_TEST(l_it.render_ms_total_ == 24690);
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(l_session.failure_reason_.empty());

  for (std::size_t l_i = 0; l_i != 2; ++l_i) {
    const auto l_cand = l_fx.reload_candidate(l_i);
    BOOST_TEST(en(l_cand.render_status_) == en(candidate_render_status::rendered));
    BOOST_TEST(l_cand.render_ms_ == 12345);
    BOOST_TEST(l_cand.light_count_ == 3);
    // 行上的 metrics_ 是 JSON 列（ORM 不支持嵌套结构体），读的时候走类型化访问器
    BOOST_TEST(l_cand.get_metrics().contrast_ == 0.42);
  }
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_keeps_paths_from_db, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(ok_report(l_fx, 1)));

  // 回传报文里不带路径（D27）：产物路径只能来自派发那一轮写进库里的那份。
  // 这里断言它一字不变 —— 一旦有人又拿回传内容去改路径，这条立刻红。
  const auto l_cand = l_fx.reload_candidate(0);
  BOOST_TEST(l_cand.render_png_path_.generic_string() == std::string{"AI_Light/it_001/cand_00.png"});
  BOOST_TEST(l_cand.render_exr_path_.empty());
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_partial_failure_still_advances, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  // §7.5 第 9 条失败隔离：**候选级**失败（status=failed）不能让整轮失败 ——
  // 整轮失败只由轮次级 error_ 表达（UE 起不来 / 超时 / 载荷非法，见下一个用例）。
  auto l_json = ok_report(l_fx, 2);
  l_json["candidates"].at(1)["status"] = "failed";

  co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(l_json));

  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_session.failure_reason_.empty());
  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::rendered));
  // 失败的那张仍然是 failed：部分结果照原样落库，不把失败项伪造成出图。
  BOOST_TEST(en(l_fx.reload_candidate(1).render_status_) == en(candidate_render_status::failed));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_error_without_rendered_writes_failure_reason, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  auto l_json = ok_report(l_fx, 2);
  l_json["candidates"].at(0)["status"] = "failed";
  l_json["candidates"].at(1)["status"] = "failed";
  l_json["error"]                      = "UE 启动失败：DoodleLighting commandlet 未找到";

  co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(l_json));

  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();

  // D28：失败不是终态 —— 轮次状态原地不动、候选行不改，会话上留一行给人看的说明。
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::rendering));
  BOOST_TEST(!l_it.render_at_.has_value());
  BOOST_TEST(l_session.failure_reason_ == std::string{"UE 启动失败：DoodleLighting commandlet 未找到"});
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
  BOOST_TEST(l_fx.reload_candidate(0).render_ms_ == 0);
  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::pending));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_all_failed_writes_failure_reason, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 3);

  // 没有 error、也没有一张 rendered：这种回传是「跑了但全灭了」，服务端自己造说明
  auto l_json = ok_report(l_fx, 3);
  for (auto& l_c : l_json["candidates"]) l_c["status"] = "failed";

  co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(l_json));

  const auto l_session = l_fx.reload_session();
  BOOST_TEST(l_session.failure_reason_ == std::string{"轮次 1 的 3 个候选全部渲染失败"});
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::rendering));
}

// ===========================================================================
// 第三段：归属与状态校验（坏回传不能改到别人的行上）
// ===========================================================================

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_rejects_unknown_task, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  const auto l_status = catch_status([&] {
    // 直接调同步那一半：能在 co_await 之前抛的都在这里抛（回传路由的 404 就是它）
    (void)parse_round_render_report(ok_report(l_fx, 1));
    throw doodle_error{boost::beast::http::status::not_found, "probe"};
  });
  BOOST_TEST(l_status == boost::beast::http::status::not_found);

  // 真调用一次，确认未知任务号确实抛 404（而不是把哪一行悄悄改坏）
  bool l_threw = false;
  try {
    co_await apply_render_report(*l_fx.sql_, core_set::get_set().get_uuid(), parse_round_render_report(ok_report(l_fx, 1)));
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_found);
  }
  BOOST_TEST(l_threw);
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_rejects_candidate_from_another_round, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  // 拿一个**别人的**候选 uuid 混进来：必须 400，而不是「查全库找到就改」
  auto l_json = ok_report(l_fx, 1);
  l_json["candidates"].at(0)["candidate_id"] = core_set::get_set().get_uuid();

  bool l_threw = false;
  try {
    co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(l_json));
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);

  // 拒掉之后本轮不能被推着往前走
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::rendering));
  BOOST_TEST(l_fx.reload_session().failure_reason_.empty());
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_rejects_judged_iteration, fixture_t) {
  // 已经判断过的轮次不接受回填（迟到的回传不能把美术的判断推翻）
  l_fx.build(ai_light_session_state::awaiting_judgement, ai_light_iteration_state::judged, 1);

  bool l_threw = false;
  try {
    co_await apply_render_report(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, parse_round_render_report(ok_report(l_fx, 1)));
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::judged));
}

// ===========================================================================
// 失败说明：截断与未知任务
// ===========================================================================

DOODLE_FIXTURE_TEST_CASE_CORO(mark_iteration_failed_truncates_long_reason, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  // 回传里塞一段 UE 的完整 stderr 是很常见的。会话行不能因此被撑爆 ——
  // brief_text 截到 512 字节后加「已截断」标记（前端靠它知道后面还有内容）。
  const std::string l_long(600, 'x');
  co_await mark_iteration_failed(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, l_long);

  const auto l_reason = l_fx.reload_session().failure_reason_;
  const std::string l_tail{"...(已截断)"};
  BOOST_TEST(l_reason.size() == 512u + l_tail.size());
  BOOST_TEST(l_reason.ends_with(l_tail));
}

DOODLE_FIXTURE_TEST_CASE_CORO(mark_iteration_failed_unknown_task_is_404, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  bool l_threw = false;
  try {
    co_await mark_iteration_failed(*l_fx.sql_, core_set::get_set().get_uuid(), "谁也说不清为什么");
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_found);
  }
  BOOST_TEST(l_threw);
  BOOST_TEST(l_fx.reload_session().failure_reason_.empty());
}

BOOST_AUTO_TEST_SUITE_END()
