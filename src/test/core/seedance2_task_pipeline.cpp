//
// 推理任务的「执行 + 结算」管线测试：run_inference_task / make_inference_client.
//
// 为什么单独一份：
//   这条管线的调用方是推理网关（ai_light.cpp 的 inference_route）。
//   曾经的两条 AI 灯光接缝（generate_candidates /
//   structure_feedback）已随「推理归前端」删除，接缝共用管道（ai_light_seam_common.cpp）
//   也已内联进网关并删除；但管线本身**仍然活着** ——
//   网关转发前端点名的那次推理时走的就是它，扣费凭证照旧要落库。
//   纯函数部分（finalize_inference_body / parse_inference_response）由 seedance2_inference.cpp
//   覆盖，但「插入即终态 + 扣费在同一个事务里」「失败不计费」「没有 usage 时退回预扣值」
//   这几条只有把 run_inference_task 真跑一遍才能验。
//
// 怎么跑而不联网：
//   客户端是 run_inference_task 的**参数**，不是函数内部 new 出来的，所以注入一个假客户端即可。
//   唯一的约束是 seedance2_client 是 final，假客户端只能继承 ai_client_base 并实现它的
//   6 个纯虚函数（除 run_task 外都是空实现）。
//
// 顺带覆盖两条「真实实现里也从没跑过」的分支：
//   * transfer_station_client::run_task 的推理拒绝分支（它在取 IP / 发请求之前就返回失败）；
//   * make_inference_client 的后端选择与凭据传递。
//

#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_client/transfer_station_client.h>
// 只包含推理入口的轻头文件：reg.h 里有「相对 doodle_lib 根」的 include，测试目标引用不到
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

// 协程用例支持（run_coro / run_coro_on_app / DOODLE_TEST_CASE_CORO）
#include <test/test_coro.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(seedance2_task_pipeline)

namespace {

using namespace doodle;
using namespace doodle::orm;
namespace sd2 = doodle::seedance2;
using doodle::http::seedance2::ai_client_base;
using doodle::http::seedance2::make_inference_client;
using doodle::http::seedance2::run_inference_task;
using doodle::http::seedance2::seedance2_client;
using doodle::http::seedance2::transfer_station_client;

/// 枚举没有 operator<<，Boost.Test 打印不出来；比名字既绕开这个要求，失败信息也可读。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_seedance2_pipeline_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

/// 用例名当临时库标签：夹具由 BOOST_FIXTURE_TEST_CASE 默认构造（在用例体之前），
/// 拿不到外面手写的 tag，用框架的当前用例名反而更准。
std::string current_case_tag() { return boost::unit_test::framework::current_test_case().p_name; }

// ─── 假客户端 ───────────────────────────────────────────────────────────────

/// 只做一件事：把构造时给的结果原样返回，并记下收到的任务与调用次数。
///
/// 继承 ai_client_base 而不是 seedance2_client：后者是 final。基类有 5 个纯虚函数
/// (submit_task / cancel_task / query_task / download_result / collect_request_info /
/// default_consumed_tokens 中的纯虚部分)，除 submit_task 外都空实现 —— 这个代价换来了
/// 「不需要网络、不需要真 key 就能验结算逻辑」。
/// 只实现 submit_task 即可: task 的 status_ 是 preparing, run_task 会把调用转给 submit_task。
class fake_client_t final : public ai_client_base {
 public:
  explicit fake_client_t(run_task_result_t in_result, std::size_t in_default_tokens = 0)
      : ai_client_base{"https://fake.invalid", ssl_ctx()},
        result_(std::move(in_result)),
        default_tokens_(in_default_tokens) {}

  boost::asio::awaitable<run_task_result_t> submit_task(const sd2::task& in_task) override {
    ++call_count_;
    last_task_ = in_task;
    co_return result_;
  }
  boost::asio::awaitable<void> cancel_task(const std::string&) override { co_return; }
  boost::asio::awaitable<run_task_result_t> query_task(const sd2::task&) override { co_return run_task_result_t{}; }
  boost::asio::awaitable<void> download_result(run_task_result_t&) override { co_return; }
  request_info_t collect_request_info(const nlohmann::json&) const override { return request_info_t{}; }
  std::size_t default_consumed_tokens(const sd2::task_type&) const override { return default_tokens_; }

  std::size_t call_count_{0};
  sd2::task last_task_{};

 private:
  /// http_client_ssl 的构造需要一个 ssl context，但本用例从不发请求，所以用一个共享的即可。
  static boost::asio::ssl::context& ssl_ctx() {
    static boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tls_client};
    return l_ctx;
  }

  run_task_result_t result_{};
  std::size_t default_tokens_{0};
};

/// 一个成功/失败可控的假响应。
ai_client_base::run_task_result_t canned_result(sd2::task_status in_status, std::int64_t in_total_tokens) {
  ai_client_base::run_task_result_t l_result{};
  l_result.status_        = in_status;
  l_result.total_tokens_  = in_total_tokens;
  l_result.task_id_       = "resp_infer_0001";
  l_result.data_response_ = nlohmann::json{{"id", "resp_infer_0001"}, {"status", "completed"}};
  if (in_status == sd2::task_status::succeeded) {
    l_result.result_text_   = R"({"schema_version": 1, "intent": {"mood": "warm"}})";
    l_result.result_json_   = nlohmann::json{{"schema_version", 1}};
    l_result.input_tokens_  = in_total_tokens - 300;
    l_result.output_tokens_ = 300;
  } else {
    l_result.error_code_    = "SensitiveContentDetected.PolicyViolation";
    l_result.data_response_ = nlohmann::json{
        {"error", nlohmann::json{{"code", "SensitiveContentDetected.PolicyViolation"}, {"message", "内容不合规"}}}
    };
  }
  return l_result;
}

// ─── fixture ────────────────────────────────────────────────────────────────

/// 装一个 ssl context。`make_inference_client` 要拿 `core_set::ctx_ptr` 建客户端，而它只由
/// 主服务器的初始化（launch/kitsu_supplement.cpp，tlsv12_client）赋值 —— worker / 单测宿主
/// 里是空的。RAII 还原，免得影响同进程里的其它用例。
struct scoped_ssl_ctx_t {
  std::shared_ptr<boost::asio::ssl::context> saved_{};

  scoped_ssl_ctx_t() : saved_{core_set::get_set().ctx_ptr} {
    if (!core_set::get_set().ctx_ptr)
      core_set::get_set().ctx_ptr =
          std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  }
  ~scoped_ssl_ctx_t() { core_set::get_set().ctx_ptr = saved_; }

  scoped_ssl_ctx_t(const scoped_ssl_ctx_t&)            = delete;
  scoped_ssl_ctx_t& operator=(const scoped_ssl_ctx_t&) = delete;
};

/// 一张干净的临时库 + 一个有点数余额的人员。
///
/// 归属 id 指向并不存在的 project/task/ai_studio，所以构造期间关掉外键
/// （与 ai_light_state.cpp / ai_light_schema.cpp 的做法一致）。
/// 库绑到基类 io_context 的 strand 上：run_sql() 的 dispatch 落在这里，能被 run() 跑完。
///
/// 继承 coro_fixture_t 是为了让用例体直接写成协程：基类自带 io_context（ctx_）与把协程
/// 跑到底的 run()。**不要**再声明一个 ctx_，否则会遮蔽基类那个（run() 用的是基类的）。
struct fixture_t : doodle_test::coro_fixture_t {
  /// app_base 必须是第一个成员，两个理由：
  ///   1) 夹具的构造发生在用例体之前（BOOST_FIXTURE_TEST_CASE 会先默认构造夹具），而
  ///      core_set::get_set() 之类的 doodle 单例要等 app_base 构造之后才可用
  ///      （impl::self 只在 app_base 构造时赋值）；scoped_ssl_ctx_t 也依赖这一点；
  ///   2) 它最后析构，保证库与 ssl context 先拆掉 —— 与改造前的次序一致。
  app_base app_{};

  scoped_ssl_ctx_t ssl_ctx_{};
  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{temp_db(current_case_tag())};
  bool fk_was_on_{true};
  bool opened_{false};

  person user_{};

  /// 默认构造（夹具只能这样被框架构造）：开库、关外键、插一个有余额的人员 ——
  /// 这三件事每个用例都要，所以直接放在构造函数里。
  fixture_t() {
    remove_db(path_);
    storage_.open(path_);
    storage_.upgrade();
    orm_       = storage_.create_session();
    sql_       = std::make_unique<sqlite_database>(boost::asio::make_strand(ctx_), orm_);

    fk_was_on_ = orm_.pragma().foreign_keys();
    orm_.pragma().foreign_keys(false);

    user_.uuid_id_                     = core_set::get_set().get_uuid();
    user_.first_name_                  = "推理";
    user_.last_name_                   = "用户";
    user_.email_                       = "inference@doodle.test";
    user_.active_                      = true;
    user_.remaining_completion_tokens_ = 100'000;
    insert(orm_).into<person>().values(user_)();
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

  std::int64_t remaining_tokens() { return sql_->get_by_uuid<person>(user_.uuid_id_).remaining_completion_tokens_; }

  /// 后台管理器（seedance2_task_run_manager::get_task）的可见集合：
  /// 只捞 preparing / queued / running，条件与那边逐字一致。
  /// 方案 A 的前提就是终态行永远不在这个集合里；条件先放进局部变量再 where，
  /// 免得这一长串 || 被格式化成一行 180 列的代码。
  std::vector<sd2::task> tasks_visible_to_the_manager() {
    const auto l_pending = c(&sd2::task::status_) == sd2::task_status::preparing ||
                           c(&sd2::task::status_) == sd2::task_status::queued ||
                           c(&sd2::task::status_) == sd2::task_status::running;
    return select(*sql_).columns(object<sd2::task>()).from<sd2::task>().where(l_pending)().to_vector();
  }
};

/// 一份「接缝会构造出来的」推理任务：预扣值故意与真实用量不同，用来证明结算用的是响应里的值。
sd2::task make_inference_task(const person& in_user, std::int64_t in_precharge = 50'000) {
  sd2::task l_task{};
  l_task.uuid_id_           = core_set::get_set().get_uuid();
  l_task.type_              = sd2::task_type::inference;
  l_task.backend_           = sd2::task_backend::seedance2;
  l_task.user_id_           = in_user.uuid_id_;
  l_task.ai_studio_id_      = core_set::get_set().get_uuid();
  l_task.data_request_      = nlohmann::json{{"input_text", "暖一点, 柔一点"}};
  l_task.text_prompt_       = "暖一点, 柔一点";
  l_task.completion_tokens_ = in_precharge;
  return l_task;
}

}  // namespace

// ─── 成功：插入即终态 + 按 total_tokens 结算 ────────────────────────────────

DOODLE_FIXTURE_TEST_CASE_CORO(succeeded_inference_inserts_a_final_row_and_bills_total_tokens, fixture_t) {
  auto l_client = std::make_shared<fake_client_t>(canned_result(sd2::task_status::succeeded, 1500));
  auto l_task   = make_inference_task(l_fx.user_);

  co_await run_inference_task(l_task, *l_fx.sql_, l_client);

  const auto l_saved = l_fx.sql_->get_by_uuid<sd2::task>(l_task.uuid_id_);
  BOOST_CHECK_EQUAL(en(l_saved.status_), "succeeded");
  BOOST_CHECK_EQUAL(l_saved.task_id_, "resp_infer_0001");
  // 推理输出文本不再单独留档（task.result_text 已删除）—— 原始响应在 data_response_ 里。
  // canned_result 里的 result_text_ 是客户端返回结构体的字段，仍然存在且仍被 seam 用于报错摘要，
  // 但它不再落进 task 行。这里改成断言那个「唯一的留档」。
  BOOST_CHECK_EQUAL(l_saved.data_response_.at("status").get<std::string>(), "completed");
  BOOST_CHECK_EQUAL(en(l_saved.type_), "inference");
  BOOST_CHECK_EQUAL(l_saved.user_id_, l_fx.user_.uuid_id_);
  // 结束时间被写上（默认构造是 epoch，不能留着）
  BOOST_CHECK(l_saved.ended_at_.get_sys_time() > chrono::system_clock::time_point{});

  // 结算用的是响应里的 total_tokens，不是调用方给的预扣值 50000
  BOOST_CHECK_EQUAL(l_saved.completion_tokens_, 1500);
  BOOST_CHECK_EQUAL(l_fx.remaining_tokens(), 100'000 - 1500);

  // 方案 A 的前提：插入即终态，所以后台管理器（5 秒轮询）永远看不到它，不会重复提交
  BOOST_CHECK(l_fx.tasks_visible_to_the_manager().empty());

  BOOST_CHECK_EQUAL(l_client->call_count_, 1);
  BOOST_CHECK_EQUAL(en(l_client->last_task_.type_), "inference");
}

// ─── 失败：落库便于审计，但不计费 ───────────────────────────────────────────

DOODLE_FIXTURE_TEST_CASE_CORO(failed_inference_is_recorded_but_not_billed, fixture_t) {
  auto l_client = std::make_shared<fake_client_t>(canned_result(sd2::task_status::failed, 1500));
  auto l_task   = make_inference_task(l_fx.user_);

  co_await run_inference_task(l_task, *l_fx.sql_, l_client);

  const auto l_saved = l_fx.sql_->get_by_uuid<sd2::task>(l_task.uuid_id_);
  BOOST_CHECK_EQUAL(en(l_saved.status_), "failed");
  // 失败不计费：completion_tokens_ 记 0，人员的余额一点没动
  BOOST_CHECK_EQUAL(l_saved.completion_tokens_, 0);
  BOOST_CHECK_EQUAL(l_fx.remaining_tokens(), 100'000);
  // 但原始响应必须留着，否则对账/排查内容违规时什么都看不到
  BOOST_CHECK_EQUAL(
      l_saved.data_response_.at("error").at("code").get<std::string>(), "SensitiveContentDetected.PolicyViolation"
  );
  BOOST_CHECK(l_fx.tasks_visible_to_the_manager().empty());
}

// ─── 没有 usage：退回预扣值，绝不能落到 50 万的默认值 ───────────────────────

DOODLE_FIXTURE_TEST_CASE_CORO(missing_usage_falls_back_to_the_precharge_not_the_generation_default, fixture_t) {
  auto l_client = std::make_shared<fake_client_t>(canned_result(sd2::task_status::succeeded, 0));
  auto l_task   = make_inference_task(l_fx.user_, 8'000);

  co_await run_inference_task(l_task, *l_fx.sql_, l_client);

  const auto l_saved = l_fx.sql_->get_by_uuid<sd2::task>(l_task.uuid_id_);
  BOOST_CHECK_EQUAL(en(l_saved.status_), "succeeded");
  // task.completion_tokens_ 的默认值是生成任务用的 50 万（见 static_value.h），
  // 推理必须显式赋值；这里响应没带 usage，就退回调用方给的预扣值。
  BOOST_CHECK_EQUAL(l_saved.completion_tokens_, 8'000);
  BOOST_CHECK(l_saved.completion_tokens_ != doodle_config::g_max_task_completion_tokens);
  BOOST_CHECK_EQUAL(l_fx.remaining_tokens(), 100'000 - 8'000);
}

// ─── 中转站后端：不支持推理，落库失败而不是假装成功 ─────────────────────────

DOODLE_FIXTURE_TEST_CASE_CORO(transfer_station_backend_fails_before_touching_the_network, fixture_t) {
  // 用**真**客户端：transfer_station_client::submit_task 的推理分支在取 IP / 发请求之前就返回失败，
  // 所以这一条不需要网络，也正好覆盖了那条同样没跑过的分支。
  // （run_inference_task 调 run_task，而任务 status_ 是 preparing，于是转发到 submit_task。）
  ai_studio l_studio{};
  l_studio.transfer_station_key_ = "ts-key";
  auto l_client                  = make_inference_client(l_studio, sd2::task_backend::transfer_station);

  auto l_task                    = make_inference_task(l_fx.user_);
  l_task.backend_                = sd2::task_backend::transfer_station;

  co_await run_inference_task(l_task, *l_fx.sql_, l_client);

  const auto l_saved = l_fx.sql_->get_by_uuid<sd2::task>(l_task.uuid_id_);
  BOOST_CHECK_EQUAL(en(l_saved.status_), "failed");
  BOOST_CHECK_EQUAL(l_saved.completion_tokens_, 0);
  BOOST_CHECK_EQUAL(l_fx.remaining_tokens(), 100'000);
  BOOST_CHECK_EQUAL(l_saved.data_response_.at("error").get<std::string>(), "中转站后端不支持推理任务");
}

// ─── 客户端工厂：后端选择、凭据传递、每次新建 ───────────────────────────────

BOOST_AUTO_TEST_CASE(make_inference_client_picks_the_backend_and_carries_credentials) {
  app_base l_app{};
  scoped_ssl_ctx_t l_ssl{};

  ai_studio l_studio{};
  l_studio.seedance2_key_        = "sd2-key";
  l_studio.transfer_station_key_ = "ts-key";

  auto l_a                       = make_inference_client(l_studio);
  auto l_b                       = make_inference_client(l_studio);
  BOOST_CHECK(dynamic_cast<seedance2_client*>(l_a.get()) != nullptr);
  BOOST_CHECK_EQUAL(l_a->get_token(), "sd2-key");
  // 每次新建，不共享：http_client_ssl 内部只有一个 socket 且没有锁，共享给并发请求会串包
  BOOST_CHECK(l_a.get() != l_b.get());

  auto l_c = make_inference_client(l_studio, sd2::task_backend::transfer_station);
  BOOST_CHECK(dynamic_cast<transfer_station_client*>(l_c.get()) != nullptr);
  BOOST_CHECK_EQUAL(l_c->get_token(), "ts-key");

  // 签名密钥来自 kitsu_ctx_t::secret_（只有主服务器有注册表密钥）。这里不假设宿主一定有，
  // 只钉住规则本身：有就带上，没有就为空（空 = 不签名，本地/worker 走这条路）。
  // 注意这里是匿名命名空间之外，`using namespace doodle` 不生效，必须写全限定名。
  const std::string l_expected =
      g_ctx().contains<doodle::http::kitsu_ctx_t>() ? g_ctx().get<doodle::http::kitsu_ctx_t>().secret_ : std::string{};
  BOOST_CHECK_EQUAL(l_a->get_url_sign_secret(), l_expected);
  BOOST_CHECK_EQUAL(l_c->get_url_sign_secret(), l_expected);
}

// ─── 没有 ssl context：明确报错，而不是崩在 SSL_new 里 ──────────────────────

BOOST_AUTO_TEST_CASE(without_an_ssl_context_the_factory_reports_instead_of_crashing) {
  app_base l_app{};
  scoped_ssl_ctx_t l_ssl{};

  ai_studio l_studio{};
  // 复现 worker / 单测宿主那条路径：写死成 nullptr。（helper 会在作用域结束时还原）
  // 修复前这里是空指针解引用，崩在 SSL_new 内部的地址 0x8 上 —— 本用例就是被那个崩溃逼出来的。
  core_set::get_set().ctx_ptr = nullptr;
  BOOST_CHECK_THROW(make_inference_client(l_studio), doodle::doodle_error);
  BOOST_CHECK_THROW(make_inference_client(l_studio, sd2::task_backend::transfer_station), doodle::doodle_error);
}

BOOST_AUTO_TEST_SUITE_END()
