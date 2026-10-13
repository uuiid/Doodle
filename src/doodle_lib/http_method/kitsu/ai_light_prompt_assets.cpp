//
// AI 灯光：提示词素材只读出口（角色降级 §3.3）实现
//
// 见 ai_light_prompt_assets.h 顶部的理由说明。
//

#include <doodle_core/exception/exception.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light_prompt_assets.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_candidates.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_common.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_feedback.h>
#include <doodle_lib/http_method/kitsu/ai_light_seam_spec.h>
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
// 而三个 `build_*_instructions()` / `build_*_json_schema()` / `build_common_lighting_section()`
// 都住在后者里。与 ai_light.cpp:50 同一写法。
using namespace doodle::ai_light;

// 素材名。与网关的 seam 值是同一套（ai_light_gateway.cpp 的 g_seam_*），
// 前三个必须逐字一致 —— 前端拿 name 去当 seam 用，不一致会 400。
//
// `common` 是第四个，它在网关那里**不是**合法 seam（没有「只跑公共段」这种推理）。
// 所以这两个白名单是「前三个重叠、第四个不重叠」的关系，不能合并成一个常量 ——
// 合并会让网关接受 seam=common（一次注定失败的推理），或让 /prompt_assets 取不到公共段。
constexpr std::string_view g_asset_spec{"generate_spec"};
constexpr std::string_view g_asset_candidates{"generate_candidates"};
constexpr std::string_view g_asset_feedback{"structure_feedback"};
constexpr std::string_view g_asset_common{"common"};

/// 素材名 → 该素材的名字。**名字即值**（不做短名映射），所以这个函数只是白名单校验。
///
/// 为什么不做 `spec` → `generate_spec` 的短名映射：映射表本身是第五个需要维护的东西，
/// 而它唯一的好处是 URL 短一点。短名还会让「网关的 seam」与「素材的 name」看起来是
/// 两个不同的域，于是前端要写一层转换 —— 那层转换就是漂移发生的地方。
std::string_view resolve_asset_name(std::string_view in_name) {
  constexpr std::array<std::string_view, 4> l_allowed{
      g_asset_spec, g_asset_candidates, g_asset_feedback, g_asset_common
  };
  DOODLE_CHICK_HTTP(
      std::find(l_allowed.begin(), l_allowed.end(), in_name) != l_allowed.end(), bad_request,
      "prompt_assets：未知的素材名 \"{}\"（应为 {} / {} / {} / {}）", in_name, g_asset_spec, g_asset_candidates,
      g_asset_feedback, g_asset_common
  );
  return in_name;
}

/// 该素材的 instructions。三个接缝各调自己的 build_*_instructions()，
/// common 调 build_common_lighting_section()。
std::string asset_instructions(std::string_view in_name) {
  if (in_name == g_asset_spec) return build_spec_instructions();
  if (in_name == g_asset_candidates) return build_candidates_instructions();
  if (in_name == g_asset_feedback) return build_feedback_instructions();
  return build_common_lighting_section();
}

/// 该素材的 schema。**common 没有 schema → 返回 null**（不是空 object）。
///
/// null 与 `{}` 的区别是有意义的：`{}` 是一个合法但无约束的 schema，前端把它发给
/// 方舟会「成功」但完全不约束输出；null 会让前端的 `if (schema)` 分支落到该走的地方。
/// 发一个空 object 等于埋一个静默失效。
nlohmann::json asset_json_schema(std::string_view in_name) {
  if (in_name == g_asset_spec) return build_spec_json_schema();
  if (in_name == g_asset_candidates) return build_candidates_json_schema();
  if (in_name == g_asset_feedback) return build_feedback_json_schema();
  return nlohmann::json{};  // common：无 schema，序列化为 null
}

/// 该素材的 schema 名与 strict 标志。与三条接缝调 run_seam_inference 时传的**逐字一致**。
///
/// 从这三对常量能看出一个约定：schema 名就是 `ai_light_<接缝名去掉动词>`。
/// 不在这里用字符串拼（`"ai_light_" + substr(...)`）—— 拼出来的名字与接缝实际传的
/// 字面量之间没有编译器保障，而方舟侧按名字匹配 schema，错一个字符就是「静默不生效」。
/// 逐字抄下来，改动时 grep 得到。
std::string asset_schema_name(std::string_view in_name) {
  if (in_name == g_asset_spec) return "ai_light_spec";
  if (in_name == g_asset_candidates) return "ai_light_candidates";
  if (in_name == g_asset_feedback) return "ai_light_feedback";
  return {};  // common：无 schema 名
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(prompt_assets_route, get) {
  person_.check_not_outsourcer();

  // 清单按固定顺序列出（spec → candidates → feedback → common），不按字母序：
  // 这个顺序就是流水线顺序，读的人按它理解「先 spec、再 candidates、最后 feedback」。
  nlohmann::json l_assets = nlohmann::json::array();
  for (const auto& l_name : {g_asset_spec, g_asset_candidates, g_asset_feedback, g_asset_common}) {
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
