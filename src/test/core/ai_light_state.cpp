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

#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
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

/// 在自建 io_context 上跑一个协程并等待它结束，把异常原样抛回调用方。
///
/// 为什么不用 co_spawn + detached 就算了：detached 会把异常直接吞掉
/// （只调 std::terminate 或记日志），而单测需要看见「编排层抛了 501」这件事。
void run_coro(boost::asio::io_context& in_ctx, std::function<boost::asio::awaitable<void>()> in_fun) {
  std::exception_ptr l_err{};
  boost::asio::co_spawn(
      in_ctx,
      [&]() -> boost::asio::awaitable<void> {
        try {
          co_await in_fun();
        } catch (...) {
          l_err = std::current_exception();
        }
      },
      boost::asio::detached
  );
  in_ctx.run();
  // 复用同一个 io_context 跑下一个协程前必须 restart，否则 run() 立即返回。
  in_ctx.restart();
  if (l_err) std::rethrow_exception(l_err);
}

/// 定稿用的 preview_file。刻意不走 build_accepted_preview_file()：
/// 那条路要真读 PNG + 落 OpenCV 变体 + 依赖 kitsu_ctx_t 的图片目录，
/// 属于 HTTP 层的职责。这里要验的是「定稿标记与预览文件同事务」这条状态机规则，
/// 所以直接造一个形状正确的行即可。
preview_file make_probe_preview_file() {
  preview_file l_pf{};
  l_pf.uuid_id_   = core_set::get_set().get_uuid();
  l_pf.task_id_   = core_set::get_set().get_uuid();
  l_pf.name_      = "probe";
  l_pf.extension_ = "png";
  l_pf.source_    = preview_file_source_enum::auto_light_generate;
  l_pf.status_    = preview_file_statuses::ready;
  return l_pf;
}

/// 一份最小可测的 AI 灯光会话数据：会话 + Spec + 一轮 + N 个候选。
///
/// 归属 id 指向并不存在的 project/task/entity，而这几列都是 NOT NULL，
/// 所以构造期间关掉外键（与 ai_light_schema.cpp 的做法一致），析构时恢复。
struct fixture_t {
  boost::asio::io_context ctx_{};
  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{};
  bool fk_was_on_{true};

  ai_light_session session_{};
  ai_light_spec spec_{};
  ai_light_iteration iteration_{};
  std::vector<ai_light_candidate> candidates_{};

  explicit fixture_t(
      const std::string& in_tag, ai_light_session_state in_state = ai_light_session_state::awaiting_judgement,
      ai_light_iteration_state in_iter_state = ai_light_iteration_state::awaiting_judgement,
      std::int32_t in_candidate_count = g_candidate_count, bool in_with_children = true
  )
      : path_{temp_db(in_tag)} {
    remove_db(path_);
    storage_.open(path_);
    storage_.upgrade();
    orm_ = storage_.create_session();
    // 绑到自建 io_context 的 strand，而不是 storage_ 那个绑在 g_io_context() 上的
    sql_ = std::make_unique<sqlite_database>(boost::asio::make_strand(ctx_), orm_);

    fk_was_on_ = orm_.pragma().foreign_keys();
    orm_.pragma().foreign_keys(false);
    build(in_state, in_iter_state, in_candidate_count, in_with_children);
  }

  ~fixture_t() {
    orm_.pragma().foreign_keys(fk_was_on_);
    sql_.reset();
    remove_db(path_);
  }

  fixture_t(const fixture_t&)            = delete;
  fixture_t& operator=(const fixture_t&) = delete;

  void build(
      ai_light_session_state in_state, ai_light_iteration_state in_iter_state, std::int32_t in_count,
      bool in_with_children
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

    spec_.session_id_ = session_.uuid_id_;
    spec_.revision_   = 1;
    insert(orm_).into<ai_light_spec>().values(spec_)();

    iteration_.session_id_ = session_.uuid_id_;
    iteration_.iteration_  = 1;
    iteration_.spec_id_    = spec_.uuid_id_;
    iteration_.state_      = in_iter_state;
    // mark_iteration_rendered 是按 server_task_info_id 反查轮次的，所以要有个非 nil 值
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
      l_cand.render_status_   = candidate_render_status::rendered;
      l_cand.render_ms_       = 18000 + l_i * 100;
      l_cand.light_count_     = 2;
      l_cand.render_png_path_ = FSys::path{"AI_Light"} / "it_001" / fmt::format("cand_{:02}.png", l_i);
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

/// 会话 + 轮次的「当前」副本，供 apply_judgement 的就地修改使用
struct judge_args_t {
  ai_light_session session{};
  ai_light_iteration iteration{};
};

judge_args_t load_for_judge(fixture_t& in_fx) {
  return judge_args_t{in_fx.reload_session(), in_fx.reload_iteration()};
}

ai_light_judgement_t make_judgement(judgement_decision in_decision, const uuid& in_candidate_id) {
  ai_light_judgement_t l_j{};
  l_j.by_                 = core_set::get_set().get_uuid();
  l_j.at_                 = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  l_j.decision_           = in_decision;
  l_j.selected_candidate_id_ = in_candidate_id;
  return l_j;
}

/// 给会话补一张参考图。
///
/// advance_session 在蒸馏 Spec 之前会校验参考图非空（决策 1：参考必有多张），
/// 所以凡是要走到 spec 分支的用例都得先有这一张 —— 反过来，缺了它就只会拿到 400。
ai_light_reference add_reference(fixture_t& in_fx, std::int32_t in_ordinal = 0) {
  ai_light_reference l_ref{};
  l_ref.session_id_       = in_fx.session_.uuid_id_;
  l_ref.ordinal_          = in_ordinal;
  l_ref.file_path_        = FSys::path{"AI_Light"} / "ref" / fmt::format("ref_{:02}.png", in_ordinal);
  l_ref.width_            = 1920;
  l_ref.height_           = 1080;
  l_ref.stats_confidence_ = 0.0;
  insert(in_fx.orm_).into<ai_light_reference>().values(l_ref)();
  return l_ref;
}

}  // namespace

// ===========================================================================
// 人工闸门 2：三种判断各自把会话推到哪
// ===========================================================================

BOOST_AUTO_TEST_CASE(judge_accept_marks_candidate_and_session) {
  app_base l_app{};
  fixture_t l_fx{"judge_accept"};
  auto l_args = load_for_judge(l_fx);

  auto l_pf = make_probe_preview_file();
  auto l_j  = make_judgement(judgement_decision::accept, l_fx.candidates_.at(2).uuid_id_);
  l_j.feedback_axes_ = {{"key_fill_ratio", 8.0}};

  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j, l_pf);
  });

  // 会话进入终态，且判断时限被清掉 —— 过期标志因此自动变 false，
  // 不需要任何人去「清理」它（这正是把过期做成派生标志的好处）
  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::accepted));
  BOOST_TEST(!l_session.judgement_due_at_.has_value());
  BOOST_TEST(l_session.failure_reason_.empty());

  // 轮次被标记为已判断，并留下判断人与判断内容
  auto l_iter = l_fx.reload_iteration();
  // judgement 是 JSON 列：必须是对象。这里单独断言一次，是因为
  // 「写入时被序列化成了数组」这种错会以 get_judgement() 抛 type_error 的形式出现，
  // 报错信息完全看不出是哪儿写坏的。
  BOOST_TEST_MESSAGE(fmt::format("judgement 列原始值: {}", l_iter.judgement_.dump()));
  BOOST_TEST(l_iter.judgement_.is_object());
  BOOST_TEST(en(l_iter.state_) == en(ai_light_iteration_state::judged));
  BOOST_TEST(l_iter.judged_by_ == l_j.by_);
  auto l_j_back = l_iter.get_judgement();
  BOOST_TEST(l_j_back.feedback_axes_.at("key_fill_ratio") == 8.0);

  // 被选中的候选：定稿标记 + preview_file。
  //
  // 注意 is_selected_ **保持 false**：它专指「被选为下一轮基准」，而定稿没有下一轮。
  // 这是两个不同的语义，不能为了「看起来统一」而合并 ——
  // 「这一轮人挑了哪张」在定稿时读 accepted_candidate，续轮时读 selected_candidate。
  auto l_picked = l_fx.reload_candidate(2);
  BOOST_TEST(!l_picked.is_selected_);
  BOOST_TEST(l_picked.is_accepted_);
  BOOST_TEST(l_picked.is_accepted_candidate());
  BOOST_TEST(l_picked.preview_file_id_ == l_pf.uuid_id_);
  BOOST_TEST(l_picked.accepted_session_id_ == l_session.uuid_id_);

  // 其余候选不得被误标
  for (std::size_t l_i = 0; l_i != l_fx.candidates_.size(); ++l_i) {
    if (l_i == 2) continue;
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_accepted_);
  }

  // 库层查询能反查出定稿候选（前端「这个镜头定稿了哪张」就走这条）
  auto l_accepted = l_fx.sql_->get_ai_light_accepted_candidate(l_session.uuid_id_);
  BOOST_TEST(l_accepted.has_value());
  BOOST_TEST(l_accepted->uuid_id_ == l_picked.uuid_id_);
  // 而「下一轮基准」这条查询必须是空的 —— 定稿没有下一轮，混用会让续轮逻辑拿到已定稿的图
  BOOST_TEST(!l_fx.sql_->get_ai_light_selected_candidate(l_session.uuid_id_, 1).has_value());

  // 定稿的 preview_file 与候选在同一事务里落库，必须查得到
  auto l_pf_row = l_fx.sql_->get_by_uuid<preview_file>(l_pf.uuid_id_);
  BOOST_TEST(l_pf_row.name_ == "probe");
  BOOST_TEST(en(l_pf_row.source_) == en(preview_file_source_enum::auto_light_generate));
}

BOOST_AUTO_TEST_CASE(judge_continue_keeps_session_iterable) {
  app_base l_app{};
  fixture_t l_fx{"judge_continue"};
  auto l_args = load_for_judge(l_fx);

  auto l_j = make_judgement(judgement_decision::continue_iteration, l_fx.candidates_.at(4).uuid_id_);
  l_j.feedback_text_ = "还是太冷，暖一点";

  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j);
  });

  // 回到 generating：可以立刻再来一轮
  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
  BOOST_TEST(!is_terminal(l_session.state_));
  BOOST_TEST(!l_session.judgement_due_at_.has_value());

  // 续轮的起点是 is_selected 的候选，所以必须标上
  auto l_picked = l_fx.reload_candidate(4);
  BOOST_TEST(l_picked.is_selected_);
  BOOST_TEST(!l_picked.is_accepted_);
  BOOST_TEST(l_picked.accepted_session_id_.is_nil());

  // 没有定稿，就不该产生 preview_file
  BOOST_TEST(l_fx.sql_->get_ai_light_accepted_candidate(l_session.uuid_id_).has_value() == false);

  // 文本反馈原样留存（结构化是 best effort，不能丢原文）
  auto l_iter = l_fx.reload_iteration();
  BOOST_TEST(l_iter.judgement_.is_object());
  BOOST_TEST(l_iter.get_judgement().feedback_text_ == "还是太冷，暖一点");
}

BOOST_AUTO_TEST_CASE(judge_reject_all_terminates_session) {
  app_base l_app{};
  fixture_t l_fx{"judge_reject"};
  auto l_args = load_for_judge(l_fx);

  auto l_j = make_judgement(judgement_decision::reject_all, uuid{});

  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j);
  });

  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::rejected));
  BOOST_TEST(is_terminal(l_session.state_));
  BOOST_TEST(!is_waiting_human(l_session.state_));

  // 全否不该选中任何候选
  for (std::size_t l_i = 0; l_i != l_fx.candidates_.size(); ++l_i) {
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_selected_);
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_accepted_);
  }
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::judged));
}

// ===========================================================================
// 非法判断必须被挡住，且不能改动任何一行
// ===========================================================================

BOOST_AUTO_TEST_CASE(judge_on_terminal_session_is_rejected) {
  app_base l_app{};
  // 直接造一个已经是终态的会话
  fixture_t l_fx{"judge_terminal", ai_light_session_state::accepted};
  auto l_args = load_for_judge(l_fx);

  auto l_j = make_judgement(judgement_decision::continue_iteration, l_fx.candidates_.at(1).uuid_id_);

  bool l_threw = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);

  // 会话仍是终态，轮次仍是待判断，候选一个都没动
  BOOST_TEST(en(l_fx.reload_session().state_) == en(ai_light_session_state::accepted));
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::awaiting_judgement));
  for (std::size_t l_i = 0; l_i != l_fx.candidates_.size(); ++l_i) {
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_selected_);
  }
}

BOOST_AUTO_TEST_CASE(judge_accept_without_preview_file_is_rejected) {
  app_base l_app{};
  fixture_t l_fx{"judge_accept_nopf"};
  auto l_args = load_for_judge(l_fx);

  // 定稿却不带 preview_file：说明调用方漏了文件动作，必须报错而不是「定稿成功但没图」
  auto l_j = make_judgement(judgement_decision::accept, l_fx.candidates_.at(0).uuid_id_);

  bool l_threw = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::internal_server_error);
  }
  BOOST_TEST(l_threw);
  BOOST_TEST(en(l_fx.reload_session().state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(!l_fx.reload_candidate(0).is_accepted_);
}

BOOST_AUTO_TEST_CASE(judge_selecting_candidate_from_another_iteration_is_rejected) {
  app_base l_app{};
  fixture_t l_fx{"judge_cross_iter"};

  // 造第 2 轮的一个候选，然后用第 1 轮的 iteration 去判断它 —— 模拟「在第 3 轮
  // 点了第 2 轮的图」这种跨轮误选
  ai_light_candidate l_other{};
  l_other.iteration_id_    = core_set::get_set().get_uuid();
  l_other.session_id_      = l_fx.session_.uuid_id_;
  l_other.candidate_index_ = 0;
  l_other.render_status_   = candidate_render_status::rendered;
  insert(l_fx.orm_).into<ai_light_candidate>().values(l_other)();

  auto l_args = load_for_judge(l_fx);
  auto l_j    = make_judgement(judgement_decision::accept, l_other.uuid_id_);
  auto l_pf   = make_probe_preview_file();

  bool l_threw = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j, l_pf);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);
  BOOST_TEST(en(l_fx.reload_session().state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(!l_fx.sql_->get_by_uuid<ai_light_candidate>(l_other.uuid_id_).is_accepted_);
}

// ===========================================================================
// 4 小时挂起
// ===========================================================================

BOOST_AUTO_TEST_CASE(awaiting_judgement_sets_due_at_four_hours_ahead) {
  app_base l_app{};
  fixture_t l_fx{"due_at", ai_light_session_state::generating, ai_light_iteration_state::rendering};

  // 走真实的「渲染完成 → 等判断」路径，验证时限是在**这里**起算的
  // （而不是派发时起算：渲染期间美术本来就没法判断）
  const auto l_before = std::chrono::system_clock::now();
  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await mark_iteration_rendered(*l_fx.sql_, l_fx.iteration_.server_task_info_id_, l_fx.candidates_);
  });

  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(l_session.judgement_due_at_.has_value());

  const auto l_delta = l_session.judgement_due_at_->get_sys_time() - l_before;
  // 时限 = 4 小时。允许几十毫秒的执行误差，但绝不允许「没设」或「设成别的值」
  BOOST_TEST(l_delta > g_judgement_timeout - std::chrono::minutes{1});
  BOOST_TEST(l_delta < g_judgement_timeout + std::chrono::minutes{1});

  // 轮次被推到等判断，并记下了整轮耗时
  auto l_iter = l_fx.reload_iteration();
  BOOST_TEST(en(l_iter.state_) == en(ai_light_iteration_state::awaiting_judgement));
  BOOST_TEST(l_iter.render_at_.has_value());
  BOOST_TEST(l_iter.render_ms_total_ > 0);
}

BOOST_AUTO_TEST_CASE(overdue_session_stays_awaiting_judgement_and_stays_judgeable) {
  app_base l_app{};
  const auto l_now = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  fixture_t l_fx{"overdue_stays_judgeable"};

  // 把判断时限拨到 5 小时前 —— 等价于「美术已经拖了 5 小时」
  l_fx.session_.judgement_due_at_ =
      chrono::system_zoned_time{chrono::current_zone(), l_now.get_sys_time() - std::chrono::hours{5}};
  update(l_fx.orm_)
      .from<ai_light_session>()
      .set(c(&ai_light_session::judgement_due_at_) = l_fx.session_.judgement_due_at_)
      .where(c(&ai_light_session::uuid_id_) == l_fx.session_.uuid_id_)();

  // 过期是**实时派生**的标志：状态不变，读模型把标志带出去。
  // 以前这里会被后台扫描器物化成 suspended 状态。
  auto l_detail = build_session_detail(*l_fx.sql_, l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail.has_value());
  BOOST_TEST(en(l_detail->session_.state_) == en(ai_light_session_state::awaiting_judgement));
  BOOST_TEST(l_detail->is_judgement_overdue_);
  // 过期不代表「不用管了」，它仍然在等人工动作 —— 否则前端会把它从待办里漏掉
  BOOST_TEST(l_detail->is_waiting_human_);

  // ---- 关键回归 ----
  // 超时**不**能剥夺判断能力。删掉 suspended 之前，这里会撞上
  // apply_judgement 的「不在等人工判断」400，而没有任何路径能从 suspended 回到
  // awaiting_judgement —— 于是已经渲染好的 6 个候选再也挑不了，
  // 只能走 /spec 重出方案，白扔一整轮渲染。这才是删掉那个状态的理由。
  auto l_args  = load_for_judge(l_fx);
  auto l_j     = make_judgement(judgement_decision::accept, l_fx.candidates_.at(2).uuid_id_);
  auto l_pf    = make_probe_preview_file();
  bool l_threw = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j, l_pf);
    });
  } catch (const std::exception& in_err) {
    l_threw = true;
    BOOST_TEST_MESSAGE(fmt::format("超时会话判断失败: {}", in_err.what()));
  }
  BOOST_TEST(!l_threw);

  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::accepted));
  BOOST_TEST(!l_session.judgement_due_at_.has_value());

  // 判断完就不该再过期 —— 标志跟着状态走，不需要任何人去清理它。
  // 这也是删掉 suspended_at 之后仍然成立的不变量。
  auto l_after = build_session_detail(*l_fx.sql_, l_fx.session_.uuid_id_);
  BOOST_TEST(l_after.has_value());
  BOOST_TEST(!l_after->is_judgement_overdue_);
}

// ===========================================================================
// 读模型
// ===========================================================================

BOOST_AUTO_TEST_CASE(session_detail_aggregates_all_tables) {
  app_base l_app{};
  fixture_t l_fx{"detail"};

  // 补一张参考图，让聚合里每一类都有内容
  ai_light_reference l_ref{};
  l_ref.session_id_       = l_fx.session_.uuid_id_;
  l_ref.ordinal_          = 0;
  l_ref.file_path_        = FSys::path{"AI_Light"} / "ref" / "ref_00.png";
  l_ref.width_            = 1920;
  l_ref.height_           = 1080;
  l_ref.stats_confidence_ = 0.0;
  insert(l_fx.orm_).into<ai_light_reference>().values(l_ref)();

  auto l_detail = build_session_detail(*l_fx.sql_, l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail.has_value());

  BOOST_TEST(l_detail->session_.uuid_id_ == l_fx.session_.uuid_id_);
  BOOST_TEST(l_detail->references_.size() == 1);
  BOOST_TEST(l_detail->spec_history_.size() == 1);
  BOOST_TEST(l_detail->current_spec_.has_value());
  BOOST_TEST(l_detail->current_spec_->revision_ == 1);
  BOOST_TEST(l_detail->iterations_.size() == 1);
  BOOST_TEST(l_detail->candidates_.size() == static_cast<std::size_t>(g_candidate_count));
  BOOST_TEST(!l_detail->accepted_candidate_.has_value());

  // awaiting_judgement 是「在等人」，不是「已超时」—— 两者不能混为一谈
  BOOST_TEST(l_detail->is_waiting_human_);
  BOOST_TEST(!l_detail->is_judgement_overdue_);

  // 不存在的会话返回 nullopt，而不是抛异常：调用方（路由层）才好翻成 404
  BOOST_TEST(!build_session_detail(*l_fx.sql_, core_set::get_set().get_uuid()).has_value());
}

BOOST_AUTO_TEST_CASE(session_detail_reports_overdue_flag) {
  app_base l_app{};
  fixture_t l_fx{"detail_overdue"};

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
// 接缝：本轮明确返回 501，且失败不留半成品
// ===========================================================================

BOOST_AUTO_TEST_CASE(generation_seams_report_not_implemented) {
  app_base l_app{};

  // 两个接缝都必须以 501 明确报出「能力还没到」，而不是假装成功。
  // 这很重要：静默返回空结果会让上游写出一个「成功但没有候选」的会话，
  // 而 501 让前端和调用方都能一眼看出该做什么。
  spec_generation_request_t l_spec_req{};
  l_spec_req.session_id_  = core_set::get_set().get_uuid();
  l_spec_req.prompt_text_ = "冷、硬、孤独";

  candidate_generation_request_t l_cand_req{};
  l_cand_req.session_id_ = core_set::get_set().get_uuid();

  boost::asio::io_context l_ctx{};
  bool l_spec_threw = false;
  bool l_cand_threw = false;
  try {
    run_coro(l_ctx, [&]() -> boost::asio::awaitable<void> { co_await generate_spec(l_spec_req); });
  } catch (const doodle_error& in_err) {
    l_spec_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_implemented);
  }
  try {
    run_coro(l_ctx, [&]() -> boost::asio::awaitable<void> { co_await generate_candidates(l_cand_req); });
  } catch (const doodle_error& in_err) {
    l_cand_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_implemented);
  }
  BOOST_TEST(l_spec_threw);
  BOOST_TEST(l_cand_threw);

  // 文本反馈结构化也是接缝，但它是**可选**通道：调用方要能捕获 501 后降级，
  // 所以这里验的是「它抛 501」，而不是「它返回空」。
  bool l_fb_threw = false;
  try {
    run_coro(l_ctx, [&]() -> boost::asio::awaitable<void> {
      co_await structure_feedback(core_set::get_set().get_uuid(), "暖一点、柔一点");
    });
  } catch (const doodle_error& in_err) {
    l_fb_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_implemented);
  }
  BOOST_TEST(l_fb_threw);
}

BOOST_AUTO_TEST_CASE(advance_session_rolls_back_to_entry_state_on_seam_failure) {
  app_base l_app{};
  // draft + 有提示词 + 有参考图 → advance_session 会走 generate_spec，而那是 501。
  // 刻意不要子行（Spec/轮次/候选）：这样「什么都没写进去」才能被干净地断言出来。
  fixture_t l_fx{"advance_rollback", ai_light_session_state::draft, ai_light_iteration_state::generated, 0, false};

  // 参考图是 spec 分支的前置条件；同时它也是这条用例的第二个观察点 ——
  // 回滚承诺「不会丢已上传的参考图」，而参考图挂在会话上（cascade），
  // 所以回滚只能改会话行，绝不能碰参考图。这一点此前没有被断言过。
  const auto l_ref = add_reference(l_fx);

  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::draft));

  bool l_threw = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await advance_session(*l_fx.sql_, l_session);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_implemented);
  }
  BOOST_TEST(l_threw);

  // 关键断言：状态回到入口状态，且**没有**生成任何 Spec —— 半成品比失败更糟
  auto l_after = l_fx.reload_session();
  BOOST_TEST(en(l_after.state_) == en(ai_light_session_state::draft));
  BOOST_TEST(!l_after.failure_reason_.empty());
  BOOST_TEST(l_fx.sql_->get_ai_light_specs(l_after.uuid_id_).empty());
  BOOST_TEST(l_fx.sql_->get_ai_light_iterations(l_after.uuid_id_).empty());

  // 参考图必须还在（回滚只写会话行）
  auto l_refs_after = l_fx.sql_->get_ai_light_references(l_after.uuid_id_);
  BOOST_TEST(l_refs_after.size() == 1);
  BOOST_TEST(l_refs_after.front().uuid_id_ == l_ref.uuid_id_);
}

BOOST_AUTO_TEST_CASE(advance_session_from_draft_without_references_is_rejected) {
  app_base l_app{};
  // 有提示词、有 Spec 之外的什么都没关系 —— 就是没有参考图
  fixture_t l_fx{"draft_no_refs", ai_light_session_state::draft, ai_light_iteration_state::generated, 0, false};

  auto l_session = l_fx.reload_session();
  bool l_threw   = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await advance_session(*l_fx.sql_, l_session);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    // 必须是 400（入参不齐），**不是** 501 —— 501 会让人以为只差接缝实现
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);

  // 校验刻意放在 try 之外：不走回滚，所以 failure_reason_ 必须仍然是空的。
  // 否则美术会在会话页上看到一条看起来像「AI 失败」的提示，而其实只是没传图。
  auto l_after = l_fx.reload_session();
  BOOST_TEST(en(l_after.state_) == en(ai_light_session_state::draft));
  BOOST_TEST(l_after.failure_reason_.empty());
  BOOST_TEST(l_fx.sql_->get_ai_light_specs(l_after.uuid_id_).empty());
}

BOOST_AUTO_TEST_CASE(advance_session_redistills_from_awaiting_spec_confirm) {
  app_base l_app{};
  // 已经有 Spec revision 1、正处于「等人工确认」—— 也就是美术看完第一版才发现没说清的状态
  fixture_t l_fx{
      "redistill", ai_light_session_state::awaiting_spec_confirm, ai_light_iteration_state::generated, 0, true
  };
  add_reference(l_fx);

  auto l_session = l_fx.reload_session();
  bool l_threw   = false;
  try {
    run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
      co_await advance_session(*l_fx.sql_, l_session);
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    // 这条断言就是本用例的全部意义：awaiting_spec_confirm 被 spec 分支接受（501），
    // 而不是掉进「当前没有可推进的步骤」的 400。没有它，「重出方案」就是空话 ——
    // /spec 只有第一次（draft）能成功。
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_implemented);
  }
  BOOST_TEST(l_threw);

  // 接缝在写库**之前**抛，所以不该留下半成品 Spec；状态回滚到入口状态
  auto l_after = l_fx.reload_session();
  BOOST_TEST(en(l_after.state_) == en(ai_light_session_state::awaiting_spec_confirm));
  BOOST_TEST(l_fx.sql_->get_ai_light_specs(l_after.uuid_id_).size() == 1);
}

// ===========================================================================
// 预设库
// ===========================================================================

BOOST_AUTO_TEST_CASE(preset_crud_roundtrip) {
  app_base l_app{};
  fixture_t l_fx{"preset_crud"};

  // 创建
  ai_light_preset l_preset{};
  l_preset.name_          = "伦勃朗";
  l_preset.tags_          = {"单灯", "侧逆", "古典"};
  l_preset.source_        = preset_source::artist;
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
  BOOST_TEST(en(l_all.front().source_) == en(preset_source::artist));
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
  l_j.decision_              = judgement_decision::continue_iteration;
  l_j.selected_candidate_id_ = core_set::get_set().get_uuid();
  l_j.feedback_axes_         = {{"kelvin", 4300.0}};

  nlohmann::json l_j_json = l_j;
  BOOST_TEST(l_j_json.is_object());
  BOOST_TEST(l_j_json.at("at").is_string());
  // 有值的时间字段必须是字符串，不是数组
  BOOST_TEST(!l_j_json.at("at").is_array());
  // 往返回来仍相等
  auto l_j_back = l_j_json.get<ai_light_judgement_t>();
  BOOST_TEST(en(l_j_back.decision_) == en(l_j.decision_));
  BOOST_TEST(l_j_back.at_.has_value());
  BOOST_TEST(l_j_back.feedback_axes_.at("kelvin") == 4300.0);

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

  // 详情读模型里的两个 optional 结构也必须是对象/null，不能是数组
  ai_light_session_detail_t l_detail{};
  l_detail.session_ = l_session;
  nlohmann::json l_detail_json = l_detail;
  BOOST_TEST(l_detail_json.at("current_spec").is_null());
  BOOST_TEST(l_detail_json.at("accepted_candidate").is_null());
  // 每轮候选数是唯一数字来源，读模型要把它带出去（前端不自己写死 6）
  BOOST_TEST(l_detail_json.at("candidate_count_per_iteration").get<std::int32_t>() == g_candidate_count);
}

BOOST_AUTO_TEST_SUITE_END()
