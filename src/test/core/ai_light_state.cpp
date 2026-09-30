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

  // 会话回到 generating（**没有终态**了），判断时限被清掉 —— 过期标志因此自动变 false，
  // 不需要任何人去「清理」它（这正是把过期做成派生标志的好处）。
  // 回到 generating 也意味着 /iterate 立即可用：定稿不是终点，美术还能接着调。
  auto l_session = l_fx.reload_session();
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
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

  // 被选中的候选：定稿标记 + preview_file + **同时是下一轮基准**。
  //
  // is_selected_ 与 is_accepted_ 仍然是两个语义（前者 = 下一轮基准，后者 = 当前定稿），
  // 只是定稿不再是终点，所以两者现在可以在同一行上同时为真（§15.10 决定 D20）：
  // 定稿之后接着调时，下一轮应当从定稿那套 rig 出发，而不是回到 LLM 的原始基线。
  auto l_picked = l_fx.reload_candidate(2);
  BOOST_TEST(l_picked.is_selected_);
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
  // 「下一轮基准」这条查询指向**同一张** —— 定稿之后接着调，起点就是这张（决定 D20）。
  // 它也是 advance_session 的 generating 分支在下一轮取 warm start 的入口。
  auto l_warm = l_fx.sql_->get_ai_light_selected_candidate(l_session.uuid_id_, 1);
  BOOST_TEST(l_warm.has_value());
  BOOST_TEST(l_warm->uuid_id_ == l_picked.uuid_id_);

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

BOOST_AUTO_TEST_CASE(judge_reject_all_keeps_session_iterable) {
  app_base l_app{};
  fixture_t l_fx{"judge_reject"};
  auto l_args = load_for_judge(l_fx);

  auto l_j = make_judgement(judgement_decision::reject_all, uuid{});

  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j);
  });

  auto l_session = l_fx.reload_session();
  // 全否**不再终止会话**：它只表示「这一轮全部不要」，会话照常可以再出一轮。
  // 区别在动作层（全否不许选候选），不在会话状态。
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
  BOOST_TEST(!is_waiting_human(l_session.state_));

  // 全否不该选中任何候选
  for (std::size_t l_i = 0; l_i != l_fx.candidates_.size(); ++l_i) {
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_selected_);
    BOOST_TEST(!l_fx.reload_candidate(l_i).is_accepted_);
  }
  BOOST_TEST(en(l_fx.reload_iteration().state_) == en(ai_light_iteration_state::judged));
}

// ===========================================================================
// 定稿是可移动的单指针：再次定稿会顶掉上一次，但旧交付物不回溯删除
// ===========================================================================

BOOST_AUTO_TEST_CASE(judge_accept_moves_the_accepted_pointer) {
  app_base l_app{};
  fixture_t l_fx{"judge_accept_move"};
  auto l_args = load_for_judge(l_fx);

  // 先造一个「上一次定稿」的候选（同会话、另一轮）+ 已注册的 preview_file。
  // 会话没有终态之后「再一次定稿」是常态：今天定稿，明天导演改意见，回来接着调。
  // 外键在 fixture 构造期间被关掉了（归属 id 指向不存在的 project/task/entity），
  // 所以这里的 iteration_id 可以指向一个不存在的轮次 —— 与
  // judge_selecting_candidate_from_another_iteration_is_rejected 同一手法。
  auto l_old_pf = make_probe_preview_file();
  insert(l_fx.orm_).into<preview_file>().values(l_old_pf)();

  ai_light_candidate l_old{};
  l_old.iteration_id_    = core_set::get_set().get_uuid();
  l_old.session_id_      = l_fx.session_.uuid_id_;
  l_old.candidate_index_ = 0;
  l_old.render_status_   = candidate_render_status::rendered;
  l_old.mark_accepted();
  l_old.preview_file_id_ = l_old_pf.uuid_id_;
  insert(l_fx.orm_).into<ai_light_candidate>().values(l_old)();

  // 前置：库里确实已经有一个定稿，否则下面的用例会「因为没东西可顶」而空转
  auto l_before = l_fx.sql_->get_ai_light_accepted_candidate(l_fx.session_.uuid_id_);
  BOOST_TEST(l_before.has_value());
  BOOST_TEST(l_before->uuid_id_ == l_old.uuid_id_);

  // 现在定稿候选 2。若少了「先解除上一次定稿」这一步，这里会直接撞
  // accepted_session_id 上的唯一索引（sqlite_orm_exception），根本走不到下面的断言 ——
  // 这正是本用例盯住的回归。
  auto l_pf = make_probe_preview_file();
  auto l_j  = make_judgement(judgement_decision::accept, l_fx.candidates_.at(2).uuid_id_);
  run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
    co_await apply_judgement(*l_fx.sql_, l_args.session, l_args.iteration, l_j, l_pf);
  });

  // 旧那一行的定稿指针被摘掉……
  auto l_old_back = l_fx.sql_->get_by_uuid<ai_light_candidate>(l_old.uuid_id_);
  BOOST_TEST(!l_old_back.is_accepted_);
  BOOST_TEST(l_old_back.accepted_session_id_.is_nil());
  BOOST_TEST(l_old_back.preview_file_id_.is_nil());
  BOOST_TEST(!l_old_back.is_accepted_candidate());
  // ……但它的 preview_file 行还在：那是已经交付过的一张图，不能回溯删除
  BOOST_TEST(l_fx.sql_->get_by_uuid<preview_file>(l_old_pf.uuid_id_).uuid_id_ == l_old_pf.uuid_id_);

  // 新定稿唯一且正确
  auto l_new = l_fx.reload_candidate(2);
  BOOST_TEST(l_new.is_accepted_);
  BOOST_TEST(l_new.is_accepted_candidate());
  BOOST_TEST(l_new.accepted_session_id_ == l_fx.session_.uuid_id_);
  BOOST_TEST(l_new.preview_file_id_ == l_pf.uuid_id_);

  auto l_after = l_fx.sql_->get_ai_light_accepted_candidate(l_fx.session_.uuid_id_);
  BOOST_TEST(l_after.has_value());
  BOOST_TEST(l_after->uuid_id_ == l_new.uuid_id_);

  // 会话照常可以再出一轮（没有终态）
  BOOST_TEST(en(l_fx.reload_session().state_) == en(ai_light_session_state::generating));
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
  BOOST_TEST(en(l_session.state_) == en(ai_light_session_state::generating));
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
// 接缝的第一道闸门：解析不出触发人就无法解析工作室、更无法计费
// （generate_spec 见 §15.7，generate_candidates 见 §15.8，structure_feedback 见 §15.9）
// ===========================================================================

/// structure_feedback 的第一道闸门。三条接缝共用 make_seam_client()，所以这里验的是
/// 「共用样板的第一道判定确实在**联网之前**发生」—— 500 与 400 的区别会决定 `/judge`
/// 是「降级继续」还是「整次判断失败」。
/// 三条接缝落地后已经没有 501 端点，所以那条「未实现接缝报 501」的用例已被它替换。
BOOST_AUTO_TEST_CASE(structure_feedback_rejects_unresolvable_operator) {
  app_base l_app{};
  fixture_t l_fx{"feedback_operator", ai_light_session_state::awaiting_judgement,
                 ai_light_iteration_state::awaiting_judgement, 1, true};

  auto l_session = l_fx.reload_session();

  // 1) operator_id 为 nil → 400（不是 500，也不是「静默记到别人头上」）
  {
    feedback_structuring_request_t l_req{};
    l_req.session_id_      = l_session.uuid_id_;
    l_req.feedback_text_   = "暖一点、柔一点";
    l_req.operator_id_     = uuid{};
    bool l_threw           = false;
    try {
      run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
        co_await structure_feedback(*l_fx.sql_, l_req);
      });
    } catch (const doodle_error& in_err) {
      l_threw = true;
      BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
    }
    BOOST_TEST(l_threw);
  }

  // 2) operator_id 指向一个不存在的 person → 也是 400
  {
    feedback_structuring_request_t l_req{};
    l_req.session_id_    = l_session.uuid_id_;
    l_req.feedback_text_ = "暖一点、柔一点";
    l_req.operator_id_   = core_set::get_set().get_uuid();
    bool l_threw         = false;
    try {
      run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> {
        co_await structure_feedback(*l_fx.sql_, l_req);
      });
    } catch (const doodle_error& in_err) {
      l_threw = true;
      BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
    }
    BOOST_TEST(l_threw);
  }
}
BOOST_AUTO_TEST_CASE(generate_spec_rejects_unresolvable_operator) {
  app_base l_app{};
  fixture_t l_fx{"spec_operator", ai_light_session_state::draft, ai_light_iteration_state::generated, 0, false};
  add_reference(l_fx);

  auto l_session = l_fx.reload_session();

  // 1) operator_id 为 nil 且会话也没有 submitter → 400（不是 500，也不是「静默记到别人头上」）
  l_session.submitter_ = uuid{};
  {
    spec_generation_request_t l_req{};
    l_req.session_id_  = l_session.uuid_id_;
    l_req.prompt_text_ = "冷、硬、孤独";
    bool l_threw       = false;
    try {
      run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> { co_await generate_spec(*l_fx.sql_, l_req); });
    } catch (const doodle_error& in_err) {
      l_threw = true;
      BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
    }
    BOOST_TEST(l_threw);
  }

  // 2) operator_id 指向一个不存在的 person → 也是 400
  {
    spec_generation_request_t l_req{};
    l_req.session_id_    = l_session.uuid_id_;
    l_req.operator_id_   = core_set::get_set().get_uuid();
    l_req.prompt_text_   = "冷、硬、孤独";
    bool l_threw         = false;
    try {
      run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> { co_await generate_spec(*l_fx.sql_, l_req); });
    } catch (const doodle_error& in_err) {
      l_threw = true;
      BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
    }
    BOOST_TEST(l_threw);
  }
}

BOOST_AUTO_TEST_CASE(advance_session_rolls_back_to_entry_state_on_seam_failure) {
  app_base l_app{};
  // draft + 有提示词 + 有参考图 → advance_session 会走 generate_spec。
  // 而 fixture 的 submitter_ 是个不存在的 person，所以接缝在**联网之前**就以
  // bad_request 失败 —— 正好用来验「接缝抛错 → 回滚」这条契约。
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
      // operator_id 传 nil → 由 advance_session 回落到 session.submitter_（不存在的 person）
      co_await advance_session(*l_fx.sql_, l_session, uuid{});
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
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
      co_await advance_session(*l_fx.sql_, l_session, uuid{});
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
      co_await advance_session(*l_fx.sql_, l_session, uuid{});
    });
  } catch (const doodle_error& in_err) {
    l_threw = true;
    // 400 在这里**不足以**说明问题：状态机「没有可推进的步骤」也是 400。
    // 真正要证的是「awaiting_spec_confirm 被 spec 分支接住、真的去蒸馏了」，
    // 所以下面还要看 failure_reason_ 里是谁在报错。fixture 的 submitter_ 是
    // 个不存在的 person，所以接缝必然以 bad_request 收场 —— 那正是「进去了」的证据。
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_request);
  }
  BOOST_TEST(l_threw);

  // 接缝在写库**之前**抛，所以不该留下半成品 Spec；状态回滚到入口状态
  auto l_after = l_fx.reload_session();
  BOOST_TEST(en(l_after.state_) == en(ai_light_session_state::awaiting_spec_confirm));
  BOOST_TEST(l_fx.sql_->get_ai_light_specs(l_after.uuid_id_).size() == 1);
  // 这条断言就是本用例的全部意义：失败原因必须来自**接缝**（解析不出触发人），
  // 而不是状态机那句「当前没有可推进的步骤」。没有它，「重出方案」就是空话 ——
  // /spec 只有第一次（draft）能成功，而两种失败又都是 400，状态码分不出来。
  BOOST_TEST(l_after.failure_reason_.find("没有可推进的步骤") == std::string::npos);
  BOOST_TEST(!l_after.failure_reason_.empty());
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
BOOST_AUTO_TEST_CASE(pre_assigned_uuid_survives_the_deferred_insert) {
  // 与本文件其它用例一样，必须先有一个 app_base：夹具里 core_set::get_set() 之类
  // 都要经过 doodle_lib 的单例（impl::self 只在 app_base 构造时才被赋值），
  // 少了它这里会在夹具构造阶段就空指针崩掉。
  app_base l_app{};
  fixture_t l_f{"preset_uuid"};
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
  l_ref.session_id_     = l_f.session_.uuid_id_;
  l_ref.ordinal_        = 99;  // 夹具不造参考图；取个大值，将来夹具扩充也不会撞 (session_id, ordinal) 唯一索引
  l_ref.width_          = 1920;
  l_ref.height_         = 1080;
  l_ref.file_path_      = l_ctx.get_ai_light_reference_file(l_f.session_.uuid_id_, l_ref.uuid_id_);
  const auto l_ref_path = l_ref.file_path_;

  // --- 任务 + 轮次：轮次引用任务的 uuid ---
  server_task_info l_task{};
  l_task.uuid_id_     = l_task_uuid;
  l_task.name_        = "probe";
  l_task.status_      = server_task_info_status::submitted;
  l_task.submitter_   = l_f.session_.submitter_;
  l_task.submit_time_ = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_task.task_id_     = l_f.session_.task_id_;
  l_task.type_        = server_task_info_type::ai_light;

  ai_light_iteration l_it{};
  l_it.uuid_id_             = l_it_uuid;
  l_it.session_id_          = l_f.session_.uuid_id_;
  l_it.iteration_           = 2;  // 夹具已用第 1 轮
  l_it.spec_id_             = l_f.spec_.uuid_id_;
  l_it.server_task_info_id_ = l_task.uuid_id_;  // 必须在任务 uuid 定下来之后

  // --- 候选：iteration_id_ 指向轮次，两个渲染路径都用自己的 uuid ---
  ai_light_candidate l_cand{};
  l_cand.uuid_id_         = l_cand_uuid;
  l_cand.iteration_id_    = l_it.uuid_id_;
  l_cand.session_id_      = l_f.session_.uuid_id_;
  l_cand.candidate_index_ = 0;
  l_cand.render_status_   = candidate_render_status::pending;
  l_cand.render_png_path_ = l_ctx.get_ai_light_candidate_file(l_f.session_.uuid_id_, 2, l_cand.uuid_id_, false);
  l_cand.render_exr_path_ = l_ctx.get_ai_light_candidate_file(l_f.session_.uuid_id_, 2, l_cand.uuid_id_, true);
  const auto l_png_path   = l_cand.render_png_path_;
  const auto l_exr_path   = l_cand.render_exr_path_;

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_f.orm_).into<ai_light_reference>().values(l_ref));
  l_sqls.emplace_back(insert(l_f.orm_).into<server_task_info>().values(l_task));
  l_sqls.emplace_back(insert(l_f.orm_).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(insert(l_f.orm_).into<ai_light_candidate>().values(l_cand));

  // 1) 回填只补 nil：预分配的一个都不许动
  BOOST_TEST(l_ref.uuid_id_ == l_ref_uuid);
  BOOST_TEST(l_task.uuid_id_ == l_task_uuid);
  BOOST_TEST(l_it.uuid_id_ == l_it_uuid);
  BOOST_TEST(l_cand.uuid_id_ == l_cand_uuid);

  run_coro(l_f.ctx_, [&]() -> boost::asio::awaitable<void> { co_await l_f.sql_->run_sql(std::move(l_sqls)); });

  // 2) 参考图：库里的 uuid 必须能重算出磁盘上那个文件名（夹具不造参考图，所以只有这一行）
  const auto l_refs = l_f.sql_->get_ai_light_references(l_f.session_.uuid_id_);
  BOOST_REQUIRE(l_refs.size() == 1);
  BOOST_TEST(l_refs.front().uuid_id_ == l_ref_uuid);
  BOOST_TEST(l_refs.front().file_path_ == l_ref_path);
  BOOST_TEST(
      l_ctx.get_ai_light_reference_file(l_f.session_.uuid_id_, l_refs.front().uuid_id_) == l_refs.front().file_path_
  );

  // 3) 轮次：按任务 uuid 能反查回来（mark_iteration_rendered 的入口）
  const auto l_it_back = l_f.sql_->get_ai_light_iteration_by_task(l_task_uuid);
  BOOST_REQUIRE(l_it_back.has_value());
  BOOST_TEST(l_it_back->uuid_id_ == l_it_uuid);
  BOOST_TEST(l_it_back->server_task_info_id_ == l_task_uuid);

  // 4) 候选：iteration_id_ 就是预分配的轮次 uuid（「候选属于本轮」的校验靠它）
  const auto l_cands = l_f.sql_->get_ai_light_candidates(l_it_uuid);
  BOOST_REQUIRE(l_cands.size() == 1);
  BOOST_TEST(l_cands.front().uuid_id_ == l_cand_uuid);
  BOOST_TEST(l_cands.front().iteration_id_ == l_it_uuid);
  BOOST_TEST(l_cands.front().render_png_path_ == l_png_path);
  BOOST_TEST(l_cands.front().render_exr_path_ == l_exr_path);
  BOOST_TEST(
      l_ctx.get_ai_light_candidate_file(l_f.session_.uuid_id_, 2, l_cands.front().uuid_id_, false) == l_png_path
  );
  BOOST_TEST(
      l_ctx.get_ai_light_candidate_file(l_f.session_.uuid_id_, 2, l_cands.front().uuid_id_, true) == l_exr_path
  );
}

BOOST_AUTO_TEST_SUITE_END()
