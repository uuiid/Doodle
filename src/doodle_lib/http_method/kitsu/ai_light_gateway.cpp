//
// AI 灯光：推理网关处理函数（角色降级 §3.1）
//
// 见 docs/ue_ai_lighting_cpp_plan.md §3.1。
//
// 本文件只做三件事：权限 → 调 run_seam_inference → 组响应。
// **不组装提示词、不解释模型输出、不碰轴** —— 那三件事降级后都在前端。
//
// 请求体**原样透传**（见 build_gateway_inference_request）：除 `model`（服务端强制）外，
// 前端给的键一律原样发往方舟 `/responses`。所以新增字段、用上 tools / text.format /
// 多模态输入都不需要动这个文件。前端请求体的形状以火山方舟 Responses API 为准
// （input / instructions / text.format ...），不再是本系统自己那套扁平键
// （input_text / json_schema）：旧扁平键按契约**静默忽略**。
//

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light_gateway.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace doodle::http::ai_light {

namespace {

// 本文件在 `doodle::http::ai_light` 里，而三个 `build_*_instructions()` /
// `build_*_json_schema()` 与 `run_seam_inference` / `seam_request_base_t`
// 都在 `doodle::ai_light` 里 —— 同名内层命名空间会**遮蔽**外层，直接写
// `ai_light::run_seam_inference` 会命中 `doodle::http::ai_light` 自己（找不到）。
// 与 ai_light.cpp:50 同一写法，这是既有的既定做法，不是本文件新造的。
using namespace doodle::ai_light;

/// 台账上「这次推理问了什么」的摘要上限。
///
/// `sd2::task.text_prompt_` 是给人看的（会话列表 + 计费台账），不是解析契约，而透传模式下
/// 前端给的 `input` 可以任意大（几十张参考图 + 长提示词）。不截断就等于把整份提示词复制进
/// 任务行 —— 那是**第二份真相**，会随会话规模线性膨胀，且与 data_request_ 里的原文可能不一致。
/// 512 字符足够认出「这是哪一次」，超出部分用显式标记表示被截断。
constexpr std::size_t g_input_preview_max{512};

/// 从 Responses 的 `input` 抽一段可读摘要, 写进任务行的 `text_prompt_`。
///
/// 两种形状都认: 字符串(= 一条 user 文本), 或元素列表(取 message.content 里
/// type=="input_text" 的 text)。图片/文件/音频类元素**跳过但计数** —— 它们是素材不是
/// 提示词, 混进摘要只会变成一串路径; 计数写进摘要尾部, 好让人知道「这次还带了 3 张图」。
///
/// 抽不出文本时返回空串(调用方据此记 WARN), **不拒绝请求**: 一个只有图片的请求是合法的,
/// 而「摘要抽不出来」是服务端自己的展示问题, 不该让这次推理失败。
std::string extract_input_preview(const nlohmann::json& in_input) {
  std::string l_text{};
  std::size_t l_skipped{0};
  const auto l_append = [&](const std::string& in_piece) {
    if (in_piece.empty()) return;
    if (!l_text.empty()) l_text += "\n";
    l_text += in_piece;
  };

  if (in_input.is_string()) {
    l_append(in_input.get_ref<const std::string&>());
  } else if (in_input.is_array()) {
    for (const auto& l_item : in_input) {
      if (!l_item.is_object() || !l_item.contains("content")) {
        ++l_skipped;
        continue;
      }
      const auto& l_content = l_item.at("content");
      if (l_content.is_string()) {
        l_append(l_content.get_ref<const std::string&>());
        continue;
      }
      if (!l_content.is_array()) {
        ++l_skipped;
        continue;
      }
      for (const auto& l_part : l_content) {
        if (!l_part.is_object() || !l_part.contains("type") || !l_part.at("type").is_string()) {
          ++l_skipped;
          continue;
        }
        if (l_part.at("type").get<std::string>() == "input_text" && l_part.contains("text") &&
            l_part.at("text").is_string())
          l_append(l_part.at("text").get_ref<const std::string&>());
        else
          ++l_skipped;  // input_image / input_file / ... : 素材, 不进摘要
      }
    }
  }

  // 截断必须在**拼接完成后**做: 按元素截会留下一个看起来完整、实则半句话的提示词。
  if (l_text.size() > g_input_preview_max) {
    const auto l_original = l_text.size();
    l_text.resize(g_input_preview_max);
    l_text += fmt::format("…(已截断, 原文 {} 字符)", l_original);
  }
  if (l_skipped != 0) l_text += fmt::format(" [+{} 个非文本输入]", l_skipped);
  return l_text;
}

/// 网关 body → 落库的推理请求 json。
///
/// 这里**不逐字段理解**前端的请求: 全部键一律原样带走, 所以方舟新增字段、前端用上
/// `tools` / `text.format` / 多模态输入时, 服务端一行都不用改 —— 这正是「原样透传」要
/// 实现的东西。
///
/// 返回值**直接就是**要落进 `task.data_request_` 的那份 json: 不再先装进 inference_request_t
/// 再序列化回来。结构体往返是一份手写白名单, 会把不认识的键静默吃掉(它连 `input` 都不认识),
/// 那正是要消灭的失败模式。
///
/// `model` 也不在这里塞: 服务端强制字段由 `finalize_inference_body()` 在出网那一刻统一覆盖,
/// 于是「前端传了什么」与「服务端改成了什么」在台账与请求体上仍然分得开。
///
/// 返回 (请求 json, 台账摘要)。摘要只用于任务行的 `text_prompt_`, 与真的发出去的 body 无关。
///
/// 注意 **不再有** `input_text`/`json_schema` 这类旧扁平键的处理: 按契约它们被**静默忽略**
/// (它们对 /responses 是未知键), 前端改用 `input` / `text.format`。给一个旧键报 400 会让
/// 「只想改前端」的人被迫先动服务端, 而静默忽略的代价只是这次推理不带那个字段。
std::pair<nlohmann::json, std::string> build_gateway_inference_request(const nlohmann::json& in_json) {
  // 前端不给 input 时**不替它组装**提示词: 契约是「模板设计归前端」, 服务端只剩一个不至于
  // 必然失败的空输入(剥掉 input/instructions, 交给客户端拼一份最小合法 body)。
  const bool l_has_input = in_json.contains("input") && !in_json.at("input").is_null();

  // 复制一份再改: in_json 来自 body_, 不是本函数的所有物; 任务行要留的是"当时发出去的那份"。
  nlohmann::json l_req = in_json;
  if (!l_has_input) {
    l_req.erase("input");
    l_req.erase("instructions");
  }

  const auto l_input = l_has_input ? in_json.at("input") : nlohmann::json{};
  return {std::move(l_req), extract_input_preview(l_input)};
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  // 外包人员不能触发推理 —— 与 judge_route / iterate_route 同一口径。
  // 它花的是工作室的钱，不是「只看一眼」的只读操作。
  person_.check_not_outsourcer();

  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  // ---- 1) 权限：会话归属（路径里的 {session_id} → check_project_access）----
  //
  // 会话 id 走**路径**（/session/{session_id}/inference），与 /session/{session_id}/spec、
  // /iterate、/judge 完全同形 —— 于是「鉴权查的对象」与「URL 上操作的对象」必然同一个，
  // 不会出现「路径说 A、体里说 B，权限查了 A、计费记到 B」。
  // 也不从体里取 project_id：会话行自己就知道它属于哪个项目。
  //
  // 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，
  // 由 http_session_data 统一翻成 404（与 load_session_checked 同一口径）。
  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  // ---- 2) 组装请求体（不做入参校验：形状归方舟管）----
  //
  // 这里**没有**「必需字段」这一关了: 请求体按契约以方舟 Responses API 为准, 什么字段必需、
  // 什么值合法全由方舟判定, 服务端替它把关只会让方舟改契约时这里成为第二份真相。
  // 唯一本系统自己消费的键是 `input`, 且它只用于抽台账摘要 —— 抽不出来也不拒绝请求。
  auto [l_request, l_input_brief] = build_gateway_inference_request(l_json);
  // 摘要抽不出来时**不拒绝**请求(只有图片的请求是合法的), 但必须可发现: 台账上一条空的
  // text_prompt_ 会让「这次推理问了什么」彻底断线, 而这正是排查内容违规时唯一要看的东西。
  if (l_input_brief.empty()) {
    SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "AI 灯光推理网关：会话 {} 的请求体里没抽出文本摘要（input 形状为 {}），任务行提示词将为空",
      l_session.uuid_id_, l_json.contains("input") ? l_json.at("input").type_name() : "缺失"
    );
  }

  // ---- 3) 计费元数据 ----
  //
  // `operator_id_` **只从 JWT 取**。请求体里若有 operator_id 键会被彻底忽略 ——
  // 这是结构性保证：没有任何代码路径能让外部值流进来（见 ai_light_gateway.h 的说明）。
  // session_id 写入 base 是为了任务归属可查；分项用不到它。
  seam_request_base_t l_base{};
  l_base.operator_id_ = person_.person_.uuid_id_;
  l_base.session_id_  = l_session.uuid_id_;
  l_base.project_id_  = l_session.project_id_;

  // ---- 4) 跑推理（凭据 + 计费 + 成败判定全在 run_seam_inference 里，不复制）----
  //
  // 不再传 text_prompt_：任务行要显示的那句话由落库侧从请求体自己读（见 collect_request_info），
  // 这里再传一份摘要等于同一份提示词存两处。
  //
  // iteration_id 留 nil：网关不铸造轮次。降级后轮次由前端在 /session/{}/iteration 提交时
  // 建立，所以这次推理「属于哪一轮」在网关这里还不存在（与蒸馏阶段同形）。
  auto l_result = co_await run_seam_inference(l_sql, l_base, l_request);

  SPDLOG_LOGGER_INFO(
    g_logger_ctrl().get_http(), "AI 灯光推理网关：会话 {} 完成一次推理（{} → {} token）", l_session.uuid_id_,
    l_result.input_tokens_, l_result.output_tokens_
  );

  // ---- 5) 组响应。原始结果回传，服务端不做任何解释 ----
  co_return in_handle->make_msg(
      nlohmann::json{
          {"result", l_result.json_},
          {"input_tokens", l_result.input_tokens_},
          {"output_tokens", l_result.output_tokens_},
          {"total_tokens", l_result.total_tokens_},
      }
  );
}

}  // namespace doodle::http::ai_light
