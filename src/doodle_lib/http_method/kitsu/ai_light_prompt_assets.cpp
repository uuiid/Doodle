//
// AI 灯光：提示词素材只读出口（角色降级 §3.3）实现
//
// 见 ai_light_prompt_assets.h 顶部的理由说明。
//

#include <doodle_core/exception/exception.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light_prompt_assets.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_common.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_feedback.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace doodle::http::ai_light {

namespace {

// 理由同 ai_light_gateway.cpp：`doodle::http::ai_light` 会遮蔽 `doodle::ai_light`，
// 而两个 `build_*_instructions()` / `build_*_json_schema()` / `build_common_lighting_section()`
// 都住在后者里。与 ai_light.cpp:50 同一写法。
using namespace doodle::ai_light;

// 素材名。这个名字就是接缝 3 自组信封时用的那个 —— 前端从这里取到
// instructions/schema 后自己拼 body, 所以名字要能跟流水线的阶段对上。
//
// 它们**不再**与任何服务端白名单耦合: 网关那边已经没有 `seam` 字段了(请求按方舟 Responses
// 形状原样透传), 于是这些名字只属于本路由, 加新素材、或改这里的名字, 都不会让
// 别处开始 400。
//
// 曾经还有两个素材:
//   * `generate_spec`: 随 spec 接缝一起删除 —— 蒸馏方案完全归前端, 服务端不再提供
//     那份 instructions / schema（前端想要什么模板就自己写）。
//   * `generate_candidates`: 随候选生成降级一起删除（角色降级 · 第二梯队）——
//     候选现在归前端, 服务端只剩 structure_feedback 这一条接缝, 所以这里只出口
//     它自己的素材。前端想复用旧的候选提示词, 只能自己维护一份。
//
// `common` 不是一次推理, 它只是一段可被接缝复用的公共提示词, 因此它**没有** schema
// （asset_json_schema 返回 null 而不是 {}）。
constexpr std::string_view g_asset_feedback{"structure_feedback"};
constexpr std::string_view g_asset_common{"common"};

/// 素材名 → 该素材的名字。**名字即值**（不做短名映射），所以这个函数只是白名单校验。
///
/// 为什么不做 `feedback` → `structure_feedback` 的短名映射：映射表本身是另一个需要维护的
/// 东西，而它唯一的好处是 URL 短一点。
std::string_view resolve_asset_name(std::string_view in_name) {
  constexpr std::array<std::string_view, 2> l_allowed{g_asset_feedback, g_asset_common};
  DOODLE_CHICK_HTTP(
      std::find(l_allowed.begin(), l_allowed.end(), in_name) != l_allowed.end(), bad_request,
      "prompt_assets：未知的素材名 \"{}\"（应为 {} / {}）", in_name, g_asset_feedback, g_asset_common
  );
  return in_name;
}

/// 该素材的 instructions。接缝 3 调自己的 build_feedback_instructions()，
/// common 调 build_common_lighting_section()。
std::string asset_instructions(std::string_view in_name) {
  if (in_name == g_asset_feedback) return build_feedback_instructions();
  return build_common_lighting_section();
}

/// 该素材的 schema。**common 没有 schema → 返回 null**（不是空 object）。
///
/// null 与 `{}` 的区别是有意义的：`{}` 是一个合法但无约束的 schema，前端把它发给
/// 方舟会「成功」但完全不约束输出；null 会让前端的 `if (schema)` 分支落到该走的地方。
/// 发一个空 object 等于埋一个静默失效。
nlohmann::json asset_json_schema(std::string_view in_name) {
  if (in_name == g_asset_feedback) return build_feedback_json_schema();
  return nlohmann::json{};  // common：无 schema，序列化为 null
}

/// 该素材的 schema 名与 strict 标志。
///
/// 从这对常量能看出一个约定：schema 名就是 `ai_light_<接缝名去掉动词>`。
/// 不在这里用字符串拼（`"ai_light_" + substr(...)`）—— 拼出来的名字与接缝实际传的
/// 字面量之间没有编译器保障，而方舟侧按名字匹配 schema，错一个字符就是「静默不生效」。
/// 逐字抄下来，改动时 grep 得到。
std::string asset_schema_name(std::string_view in_name) {
  if (in_name == g_asset_feedback) return "ai_light_feedback";
  return {};  // common：无 schema 名
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(prompt_assets_route, get) {
  person_.check_not_outsourcer();

  // 清单按固定顺序列出（feedback → common），不按字母序：这个顺序就是流水线顺序。
  nlohmann::json l_assets = nlohmann::json::array();
  for (const auto& l_name : {g_asset_feedback, g_asset_common}) {
    const auto l_schema = asset_json_schema(l_name);
    l_assets.emplace_back(
        nlohmann::json{
            {"name", l_name},
            {"has_schema", !l_schema.is_null()},
            {"instructions_chars", asset_instructions(l_name).size()},
            {"schema_chars", l_schema.is_null() ? 0 : l_schema.dump().size()},
        }
    );
  }

  co_return in_handle->make_msg(nlohmann::json{{"assets", std::move(l_assets)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(prompt_asset_route, get) {
  person_.check_not_outsourcer();

  // name_ 是 FSys::path（路径段只能是 FSys::path，见头文件说明），这里收回字符串。
  // 用 generic_string() 而不是 string()：Windows 上后者会把分隔符变成反斜杠，
  // 而白名单里全是正斜杠下的普通标识符 —— 虽然两者在这些取值上恰好相同，
  // 但用 generic_string 让「URL 段」与「文件路径」这两件事在措辞上分开。
  const auto l_name = resolve_asset_name(name_.generic_string());

  co_return in_handle->make_msg(
      nlohmann::json{
          {"name", l_name},
          {"instructions", asset_instructions(l_name)},
          {"json_schema", asset_json_schema(l_name)},
          {"json_schema_name", asset_schema_name(l_name)},
          // common 没有 schema，strict 也就无意义 —— 回 false 而不是 true，
          // 免得前端读到一个「声称要严格校验、实际没有 schema」的组合。
          {"json_schema_strict", l_name != g_asset_common},
      }
  );
}

}  // namespace doodle::http::ai_light
