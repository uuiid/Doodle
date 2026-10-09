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
// 为什么专门钉一条「路径与回传无关」：回传报文里**不带**产物路径（D27），服务端只用
// 命名规则现算（D44）。这些用例断言渲染完成后候选项的三个 uuid 一字不变，因而算出来的
// 路径也一字不变 —— 那就是那条规则的回归钉子。
//
//   4. 产物存在性（D41）—— 工作机的上传是**独立一步**，所以「报告说 rendered」
//      不等于「产物取得到」。落地时逐张按命名规则算出**首帧海报**与**候选视频**再查在不在：
//      任缺一样就降级 failed（美术先扫海报、再点开视频判断，缺哪样这个候选都不成立）；
//      全都不在就走失败出口（否则美术会对着取不到的产物点「选第 3 个」）。
//      EXR 是可选的（D42）：文件不在只记一条 WARN，不降级、不阻断本轮。
//
//   5. 路径不落库（D44）—— 候选行上**没有**路径列，也没有那两个 JSON 键：
//      路径 = candidate_render_path(ctx, 候选行, linear)，纯函数，吃候选行自带的三个 uuid。
//
// 为什么不经过 HTTP 层：路由只是 get_json + parse + apply 三步，语义全在这两个函数里；
// 单测直接调它们能拿到确定的错误码。JWT / 任务类型这些代理层的东西只有真机能验，
// 不在份内（ai_light.cpp 的路由覆盖不到）。
//

#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/ai_light/task_payload.h>
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
#include <fstream>
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

/// 写一个占位文件。D41 之后「候选可判断」是靠 `FSys::exists(candidate_render_path(...))` 判的，
/// 所以夹具必须造**真文件** —— 内容无关紧要，在不在才是全部语义。
void write_probe_file(const FSys::path& in_path) {
  FSys::create_directories(in_path.parent_path());
  std::ofstream l_of{in_path, std::ios::binary | std::ios::trunc};
  l_of << 'p';
  l_of.close();
  BOOST_REQUIRE_MESSAGE(FSys::exists(in_path), "夹具占位文件没写成功：" + in_path.generic_string());
}

/// 「会话 + Spec + 一轮 + N 个候选」。归属 id 指向并不存在的 project/task/entity，
/// 而这几列都是 NOT NULL，所以构造期间关掉外键（与 ai_light_state.cpp 一致），析构时恢复。
///
/// 与 ai_light_state.cpp 的那份是同一个形状，但只留回传用例要的部分：
/// 候选行只带三个 uuid，产物路径由 candidate_render_path 现算（D44）；
/// 服务端数据根用 media_dir_，于是产物都落在临时目录里，析构时一并清掉。
struct fixture_t : doodle_test::coro_fixture_t {
  /// app_base 必须是第一个成员：core_set::get_set() 之类的单例要等它构造之后才可用，
  /// 而夹具的构造发生在用例体之前。它最后析构，保证库先拆掉。
  app_base app_{};

  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{temp_db(current_case_tag())};
  /// 候选产物文件放在这里（用例名当标签，避免并行/串行跑同一个用例时互相踩）。
  /// 清理放在析构：这些用例多数只关心「文件在不在」，谁删了谁没删都写在用例里。
  FSys::path media_dir_{
      FSys::temp_directory_path() / fmt::format("doodle_test_ai_light_dispatch_media_{}", current_case_tag())
  };
  /// 服务端数据根（kitsu_ctx_t::root_）也挂在 media_dir_ 下：产物路径于是全落在临时目录里，
  /// 析构时随 media_dir_ 一起清掉。注意不能叫 ctx_ —— 基类的 ctx_ 是 asio 的 io_context。
  http::kitsu_ctx_t server_ctx_{};
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

    // 服务端数据根 <临时目录>/…_media_<用例名>/server_root：与生产同形状（root_/ai_light/…），
    // 只是不在 D: 上。
    server_ctx_.root_ = media_dir_ / "server_root";

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
    std::error_code l_ec{};
    FSys::remove_all(media_dir_, l_ec);
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
      insert(orm_).into<ai_light_candidate>().values(l_cand)();
      candidates_.push_back(l_cand);
      // 库里没有路径可写（D44）：路径由候选行现算，所以夹具按**同一个函数**把真文件造在
      // 算出来的位置上。D41 落地后，指向不存在文件的 rendered 会被服务端降级成 failed，
      // 那是几条专门用例要构造的场景，不能混进默认形状里。
      write_probe_file(candidate_png(candidates_.size() - 1));
      write_probe_file(candidate_movie(candidates_.size() - 1));
      write_probe_file(candidate_exr(candidates_.size() - 1));
    }
  }

  /// 服务端认为候选产物**应该**在的地方（D44）：与生产代码同一个函数，测试里不自己拼字符串。
  FSys::path candidate_png(std::size_t in_index) const {
    return candidate_render_path(server_ctx_, candidates_.at(in_index), false);
  }
  FSys::path candidate_exr(std::size_t in_index) const {
    return candidate_render_path(server_ctx_, candidates_.at(in_index), true);
  }
  /// 候选视频：工作机把 MRQ 渲出来的帧合成 mp4 后上传到的地方
  FSys::path candidate_movie(std::size_t in_index) const {
    return candidate_movie_path(server_ctx_, candidates_.at(in_index));
  }

  /// 删掉某个候选的三份产物文件 —— 用来构造「报告说 rendered、产物却不在」。
  void delete_candidate_files(std::size_t in_index) {
    std::error_code l_ec{};
    FSys::remove(candidate_png(in_index), l_ec);
    FSys::remove(candidate_movie(in_index), l_ec);
    FSys::remove(candidate_exr(in_index), l_ec);
    BOOST_REQUIRE(!FSys::exists(candidate_png(in_index)));
    BOOST_REQUIRE(!FSys::exists(candidate_movie(in_index)));
  }

  /// 只删视频：海报仍在 —— 这两个是**都要在**的（D41），缺一个整条候选降级。
  void delete_candidate_movie(std::size_t in_index) {
    std::error_code l_ec{};
    FSys::remove(candidate_movie(in_index), l_ec);
    BOOST_REQUIRE(!FSys::exists(candidate_movie(in_index)));
    BOOST_REQUIRE(FSys::exists(candidate_png(in_index)));
  }

  /// 只删 EXR：PNG 仍在，用来验证「可选产物缺了不降级」（D42）。
  void delete_candidate_exr(std::size_t in_index) {
    std::error_code l_ec{};
    FSys::remove(candidate_exr(in_index), l_ec);
    BOOST_REQUIRE(!FSys::exists(candidate_exr(in_index)));
    BOOST_REQUIRE(FSys::exists(candidate_png(in_index)));
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

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 2)));

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

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_keeps_paths_derivable_from_the_row, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 1)));

  // 回传报文里不带路径（D27），候选行上也没有路径列（D44）：路径是候选行三个 uuid 的纯函数。
  // 所以这里的钉子分两半 —— 行本身（uuid 三件套）没被回传改写，且算出来的仍是夹具造真文件
  // 的那个位置。一旦有人拿回传内容去动 uuid / 改命名规则，这条立刻红。
  const auto l_cand = l_fx.reload_candidate(0);
  BOOST_TEST(l_cand.uuid_id_ == l_fx.candidates_.at(0).uuid_id_);
  BOOST_TEST(l_cand.iteration_id_ == l_fx.candidates_.at(0).iteration_id_);
  BOOST_TEST(l_cand.session_id_ == l_fx.candidates_.at(0).session_id_);
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_cand, false) == l_fx.candidate_png(0));
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_cand, true) == l_fx.candidate_exr(0));
  // 夹具造的是真文件，所以这两条同时说明「算出来的落点就是文件所在的地方」
  BOOST_TEST(FSys::exists(candidate_render_path(l_fx.server_ctx_, l_cand, false)));
  BOOST_TEST(FSys::exists(candidate_render_path(l_fx.server_ctx_, l_cand, true)));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_partial_failure_still_advances, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);

  // §7.5 第 9 条失败隔离：**候选级**失败（status=failed）不能让整轮失败 ——
  // 整轮失败只由轮次级 error_ 表达（UE 起不来 / 超时 / 载荷非法，见下一个用例）。
  auto l_json = ok_report(l_fx, 2);
  l_json["candidates"].at(1)["status"] = "failed";

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(l_json));

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

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(l_json));

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

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(l_json));

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
    co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, core_set::get_set().get_uuid(), ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 1)));
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
    co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(l_json));
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
    co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 1)));
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
  co_await mark_iteration_failed(
      *l_fx.sql_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, l_long);

  const auto l_reason = l_fx.reload_session().failure_reason_;
  const std::string l_tail{"...(已截断)"};
  BOOST_TEST(l_reason.size() == 512u + l_tail.size());
  BOOST_TEST(l_reason.ends_with(l_tail));
}

DOODLE_FIXTURE_TEST_CASE_CORO(mark_iteration_failed_unknown_task_is_404, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  bool l_threw = false;
  try {
    co_await mark_iteration_failed(
      *l_fx.sql_, core_set::get_set().get_uuid(), ai_light_iteration_locator_t{}, "谁也说不清为什么");
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_found);
  }
  BOOST_TEST(l_threw);
  BOOST_TEST(l_fx.reload_session().failure_reason_.empty());
}

// ===========================================================================
// 第四段：产物存在性（D41）
//
// 工作机的上传是**独立一步**（D40）：MRQ 写完帧、工作机合成视频与首帧海报、报告也写了
// rendered，上传那一步仍可能失败。所以「报告说渲染成功」只是主张，能不能取到产物要服务端
// 自己看文件。候选的产物是**两样**（海报 + 视频，D45），缺哪一样都不成立：美术先扫海报、
// 再点开视频判断，对着取不到的产物点「选第 3 个」，这一轮的人力和一次派发就白费了。
// ===========================================================================

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_downgrades_missing_artifacts_only_for_that_candidate, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 3);
  l_fx.delete_candidate_files(1);

  // 三条都报 rendered，但第 2 条的产物文件不在
  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 3))
  );

  // 其余候选照常推进：一个候选的产物传丢了不该让整轮作废（§7.5 第 9 条的候选级隔离）
  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_session.failure_reason_.empty());

  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::rendered));
  // 声称 rendered 但产物不在 → 降级 failed（留在库里的是「这个不可选」，不是一个 404 链接）
  BOOST_TEST(en(l_fx.reload_candidate(1).render_status_) == en(candidate_render_status::failed));
  BOOST_TEST(en(l_fx.reload_candidate(2).render_status_) == en(candidate_render_status::rendered));
  // 路径本身不动：它是「服务端应有产物的地方」（按命名规则现算，D44），不是上传结果
  const auto l_cand1 = l_fx.reload_candidate(1);
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_cand1, false) == l_fx.candidate_png(1));
  BOOST_TEST(!FSys::exists(candidate_render_path(l_fx.server_ctx_, l_cand1, false)));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_downgrades_when_only_the_movie_is_missing, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);
  l_fx.delete_candidate_movie(0);

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 2))
  );

  // 海报在、视频缺：这个候选判不了 —— 美术点开发现播不出来，等于没渲
  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::failed));
  BOOST_TEST(en(l_fx.reload_candidate(1).render_status_) == en(candidate_render_status::rendered));
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_fx.reload_session().failure_reason_.empty());
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_all_artifacts_missing_writes_failure_reason, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);
  l_fx.delete_candidate_files(0);
  l_fx.delete_candidate_files(1);

  // 全部「渲染成功」但产物一件都不在：这批回传在语义上等于什么都没渲出来，
  // 必须走失败出口 —— 推到 awaiting_judgement 就是让美术看 6 个 404。
  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 2))
  );

  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();
  BOOST_TEST(l_session.failure_reason_ == std::string{"轮次 1 的 2 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果"});
  // D28：失败不是终态 —— 轮次状态原地不动，美术再点一次 /iterate 就重派
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::rendering));
  BOOST_TEST(!l_it.render_at_.has_value());
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_missing_exr_does_not_block, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 2);
  l_fx.delete_candidate_exr(0);

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(ok_report(l_fx, 2))
  );

  // EXR 是可选产物（D42）：缺了不降级、不阻断本轮
  const auto l_it = l_fx.reload_iteration();
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_fx.reload_session().failure_reason_.empty());

  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::rendered));
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_fx.reload_candidate(0), false) == l_fx.candidate_png(0));
  // 海报与视频都在（缺 EXR 之所以不阻断，是因为这两样才是判断通道）
  BOOST_TEST(FSys::exists(candidate_movie_path(l_fx.server_ctx_, l_fx.reload_candidate(0))));

  // 库里没有 EXR 记录可清（D44）—— 那条路径本来就是现算的，所以这里钉的是：缺 EXR 之后
  // 候选行**一个字段都没被这条回传改动**，只有 render_status_ 等诊断数字按常规落库。
  const auto l_cand0 = l_fx.reload_candidate(0);
  BOOST_TEST(l_cand0.uuid_id_ == l_fx.candidates_.at(0).uuid_id_);
  BOOST_TEST(l_cand0.iteration_id_ == l_fx.candidates_.at(0).iteration_id_);
  // 旁边的 EXR 文件在，算出来的路径也指向真文件 —— 「缺 EXR 才有 WARN」的另一半对照
  BOOST_TEST(FSys::exists(candidate_render_path(l_fx.server_ctx_, l_fx.reload_candidate(1), true)));
  // 0 号的 EXR 文件确实没了：上面那条「不降级」不是因为文件其实还在
  BOOST_TEST(!FSys::exists(candidate_render_path(l_fx.server_ctx_, l_cand0, true)));
}

DOODLE_FIXTURE_TEST_CASE_CORO(apply_report_reported_failure_plus_downgrade_leaves_nothing, fixture_t) {
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 3);
  l_fx.delete_candidate_files(0);
  l_fx.delete_candidate_files(2);

  // 「报告里本来就说 failed」和「声称 rendered 但产物不在被降级」交错：两种口味的不可用
  // 混在同一条回传里，最终判定必须只看**降级之后还剩几个能看的**。
  auto l_json = ok_report(l_fx, 3);
  l_json["candidates"].at(1)["status"] = "failed";

  co_await apply_render_report(
      *l_fx.sql_, l_fx.server_ctx_, l_fx.iteration_.server_task_info_id_, ai_light_iteration_locator_t{}, parse_round_render_report(l_json));

  const auto l_it      = l_fx.reload_iteration();
  const auto l_session = l_fx.reload_session();
  // 注意计数是**2**（被降级的那两个），报告里本来就 failed 的那个不算「声称成功而缺产物」——
  // 说明写给人看的原因串时数的是「这次是哪儿出的问题」，不是候选总数。
  BOOST_TEST(l_session.failure_reason_ == std::string{"轮次 1 的 2 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果"});
  BOOST_TEST(en(l_it.state_) == en(ai_light_iteration_state::rendering));
  // 降级只发生在内存里的候选副本上，失败出口不写候选行（D28）——库里仍是 pending
  BOOST_TEST(en(l_fx.reload_candidate(0).render_status_) == en(candidate_render_status::pending));
}

// ===========================================================================
// 第五段：路径派生（D44）
//
// 候选行上没有路径列，也没有那两个 JSON 键：路径 = candidate_render_path(ctx, 行, linear)
// 与 candidate_movie_path(ctx, 行) / candidate_frames_dir(ctx, 行)，吃的都是行自带的
// session_id_ / iteration_id_ / uuid_id_ 三个 uuid。这一段盯的就是「它们确实是纯函数」——
// 同一行算两次同一个结果、不同行不撞、只有 root_ 变结果才跟着变。
// ===========================================================================

// 纯同步用例：体里一个 co_await 都没有，用 CORO 宏展开出的函数不是协程 → C4716
BOOST_FIXTURE_TEST_CASE(candidate_render_path_is_a_pure_function_of_the_row, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 3);

  const auto l_c0 = l_fx.reload_candidate(0);
  const auto l_c1 = l_fx.reload_candidate(1);

  // ① 同一行算两次一样（纯函数），PNG/EXR 同名不同扩展名
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_c0, false) == candidate_render_path(l_fx.server_ctx_, l_c0));
  BOOST_TEST(
      candidate_render_path(l_fx.server_ctx_, l_c0, false).stem() ==
      candidate_render_path(l_fx.server_ctx_, l_c0, true).stem()
  );
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_c0, false).extension().generic_string() == ".png");
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_c0, true).extension().generic_string() == ".exr");

  // 视频与帧目录是同族派生：同一个候选 uuid —— 一个是轮次目录下的 `<uuid>.mp4`，
  // 另一个是 `<uuid>/` 子目录（MRQ 把几百张 frame_*.png 写进去，不与同级文件混在一起）
  BOOST_TEST(candidate_movie_path(l_fx.server_ctx_, l_c0).filename().generic_string() == fmt::to_string(l_c0.uuid_id_) + ".mp4");
  BOOST_TEST(
      candidate_movie_path(l_fx.server_ctx_, l_c0).parent_path() ==
      candidate_frames_dir(l_fx.server_ctx_, l_c0).parent_path()
  );
  BOOST_TEST(candidate_frames_dir(l_fx.server_ctx_, l_c0).filename().generic_string() == fmt::to_string(l_c0.uuid_id_));
  BOOST_TEST(candidate_movie_path(l_fx.server_ctx_, l_c0) != candidate_render_path(l_fx.server_ctx_, l_c0, false));

  // ② 文件名是候选 uuid；父目录两级分别是「轮次 uuid」与「会话目录」
  BOOST_TEST(
      candidate_render_path(l_fx.server_ctx_, l_c0, false).filename().generic_string() ==
      fmt::to_string(l_c0.uuid_id_) + ".png"
  );
  BOOST_TEST(
      candidate_render_path(l_fx.server_ctx_, l_c0, false).parent_path().filename().generic_string() ==
      fmt::to_string(l_c0.iteration_id_)
  );
  BOOST_TEST(
      candidate_render_path(l_fx.server_ctx_, l_c0, false).parent_path().parent_path() ==
      l_fx.server_ctx_.get_ai_light_session_dir(l_c0.session_id_)
  );

  // ③ 同一轮的不同候选不撞（文件名带各自 uuid），根也参与：换个 root_ 结果整体平移
  BOOST_TEST(candidate_render_path(l_fx.server_ctx_, l_c0, false) != candidate_render_path(l_fx.server_ctx_, l_c1, false));
  http::kitsu_ctx_t l_other{};
  l_other.root_ = l_fx.media_dir_ / "another_root";
  BOOST_TEST(candidate_render_path(l_other, l_c0, false) != candidate_render_path(l_fx.server_ctx_, l_c0, false));
  BOOST_TEST(
      candidate_render_path(l_other, l_c0, false).lexically_relative(l_other.root_) ==
      candidate_render_path(l_fx.server_ctx_, l_c0, false).lexically_relative(l_fx.server_ctx_.root_)
  );
}

// ===========================================================================
// 第六段：wire 契约冻结
//
// 载荷与回传报文是**跨进程契约**：服务端在这里写载荷、工作机读它并把相对路径拼成本地绝对路径
// 喂给 UE，UE 侧还有一份手写的镜像解析（`script/uePlug/Doodle/Source/DoodleLighting/*Json.cpp`）。
// 键名一改，改的这一侧照常编译、另一侧只是静默取到默认值 —— 到真机上表现为「渲染任务莫名失败」。
// 所以键名在这里钉死：**加键是安全的**（两侧都按「缺了就默认」读），改键名必须同步改 UE 侧
// 并同时改这条用例。
// ===========================================================================

BOOST_FIXTURE_TEST_CASE(task_payload_json_keys_are_frozen, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  ai_light_task_candidate_t l_cand{};
  l_cand.candidate_index_ = 0;
  l_cand.candidate_id_    = l_fx.candidates_.at(0).uuid_id_;
  l_cand.frames_dir_      = make_candidate_frames_dir(l_cand.candidate_id_);
  l_cand.png_path_        = make_candidate_file_name(l_cand.candidate_id_, false);
  l_cand.movie_path_      = make_candidate_movie_file(l_cand.candidate_id_);

  ai_light_task_payload_t l_payload{};
  l_payload.session_id_ = l_fx.session_.uuid_id_;
  l_payload.out_dir_    = make_iteration_relative_dir(l_fx.session_.uuid_id_, l_fx.iteration_.uuid_id_);
  l_payload.candidates_.push_back(l_cand);

  // 拷贝初始化，**不要**写 nlohmann::json{l_payload}：花括号会走 initializer_list 构造，
  // 结果是一个单元素数组（`[payload]`），下面所有 contains 都会失败。
  const nlohmann::json l_j = l_payload;
  for (const auto* l_key : {"session_id", "iteration", "spec_id", "out_dir", "baseline_level", "render_map",
                            "level_sequence", "movie_pipeline_config", "frame_begin", "frame_end",
                            "ue_project_path", "camera_file_path", "candidates"}) {
    BOOST_TEST(l_j.contains(l_key));
  }

  BOOST_REQUIRE_EQUAL(l_j.at("candidates").size(), 1u);
  const auto& l_jc = l_j.at("candidates").at(0);
  for (const auto* l_key : {"candidate_index", "candidate_id", "frames_dir", "png_path", "exr_path", "movie_path", "rig"}) {
    BOOST_TEST(l_jc.contains(l_key));
  }

  // 回传只有数值与状态（D27）：多出任何一个 path 键，就说明有人在报文里塞了路径
  candidate_render_report_t l_cr{};
  l_cr.candidate_id_ = l_cand.candidate_id_;
  round_render_report_t l_report{};
  l_report.candidates_.push_back(l_cr);
  l_report.frame_rate_ = 24;

  const nlohmann::json l_jr = l_report;
  for (const auto* l_key : {"candidates", "frame_rate", "error"}) BOOST_TEST(l_jr.contains(l_key));
  const auto& l_jrc = l_jr.at("candidates").at(0);
  for (const auto* l_key :
       {"candidate_index", "candidate_id", "status", "render_ms", "light_count", "frame_count", "metrics"}) {
    BOOST_TEST(l_jrc.contains(l_key));
  }
  BOOST_TEST(!l_jrc.contains("png_path"));
  BOOST_TEST(!l_jrc.contains("movie_path"));

  // 往返一次：from_json 是宽松读（缺键即默认），所以这里同时验了键名拼写对得上
  const auto l_back = l_j.get<ai_light_task_payload_t>();
  BOOST_TEST(l_back.session_id_ == l_payload.session_id_);
  BOOST_TEST(l_back.out_dir_ == l_payload.out_dir_);
  BOOST_TEST(l_back.candidates_.size() == 1u);
  BOOST_TEST(l_back.candidates_.at(0).candidate_id_ == l_cand.candidate_id_);
  BOOST_TEST(l_back.candidates_.at(0).movie_path_ == l_cand.movie_path_);
}

// 引擎内路线（D52/D54）的返回体是**第三种 wire 形状**：本机客户端读它起 UE，UE 侧面板写的
// 桥文件也是这个结构（`script/uePlug/Doodle/Source/DoodleLighting/*Json.cpp` 手写镜像）。
// 它与工作机载荷的差别正是最容易改坏的地方：
//   1. 顶层多 `iteration_id` —— 引擎内路线没有任务行，回传只能靠「会话 + 轮次号」定位（D54），
//      轮次 uuid 与轮次号缺一个都回不去；
//   2. 顶层**没有** `spec_id` / `baseline_level` —— 那两个是工作机载荷专有的。
// 候选那一层必须与工作机载荷**逐键相同**（UE 侧共用一份候选解析）。
BOOST_FIXTURE_TEST_CASE(launch_plan_json_keys_are_frozen, fixture_t) {
  auto& l_fx = *this;
  l_fx.build(ai_light_session_state::generating, ai_light_iteration_state::rendering, 1);

  ai_light_task_candidate_t l_cand{};
  l_cand.candidate_index_ = 0;
  l_cand.candidate_id_    = l_fx.candidates_.at(0).uuid_id_;
  l_cand.frames_dir_      = make_candidate_frames_dir(l_cand.candidate_id_);
  l_cand.png_path_        = make_candidate_file_name(l_cand.candidate_id_, false);
  l_cand.movie_path_      = make_candidate_movie_file(l_cand.candidate_id_);

  ai_light_launch_plan_t l_plan{};
  l_plan.session_id_   = l_fx.session_.uuid_id_;
  l_plan.iteration_id_ = l_fx.iteration_.uuid_id_;
  l_plan.iteration_    = 1;
  l_plan.out_dir_      = make_iteration_relative_dir(l_fx.session_.uuid_id_, l_fx.iteration_.uuid_id_);
  l_plan.frame_begin_  = 0;
  l_plan.frame_end_    = 47;
  l_plan.candidates_.push_back(l_cand);

  // 同样是拷贝初始化：花括号会得到单元素数组
  const nlohmann::json l_j = l_plan;
  for (const auto* l_key : {"session_id", "iteration_id", "iteration", "out_dir", "render_map", "level_sequence",
                            "movie_pipeline_config", "frame_begin", "frame_end", "ue_project_path",
                            "camera_file_path", "candidates"}) {
    BOOST_TEST(l_j.contains(l_key));
  }
  BOOST_TEST(!l_j.contains("spec_id"));
  BOOST_TEST(!l_j.contains("baseline_level"));

  BOOST_REQUIRE_EQUAL(l_j.at("candidates").size(), 1u);
  const auto& l_jc = l_j.at("candidates").at(0);
  for (const auto* l_key : {"candidate_index", "candidate_id", "frames_dir", "png_path", "exr_path", "movie_path", "rig"}) {
    BOOST_TEST(l_jc.contains(l_key));
  }

  const auto l_back = l_j.get<ai_light_launch_plan_t>();
  BOOST_TEST(l_back.session_id_ == l_plan.session_id_);
  BOOST_TEST(l_back.iteration_id_ == l_plan.iteration_id_);
  BOOST_TEST(l_back.iteration_ == l_plan.iteration_);
  BOOST_TEST(l_back.frame_end_ == l_plan.frame_end_);
  BOOST_TEST(l_back.candidates_.size() == 1u);
  BOOST_TEST(l_back.candidates_.at(0).movie_path_ == l_cand.movie_path_);
  // 轮次号必须真能定位（D54）：拿回来的会话 + 轮次号要能在库里查到这一轮，
  // 而且查到的行就是报文里那个轮次 uuid —— 本机客户端回传时就靠这一对。
  const auto l_found = l_fx.sql_->get_ai_light_iteration(l_back.session_id_, l_back.iteration_);
  BOOST_REQUIRE(l_found.has_value());
  BOOST_TEST(l_found->uuid_id_ == l_plan.iteration_id_);
}

BOOST_AUTO_TEST_SUITE_END()

