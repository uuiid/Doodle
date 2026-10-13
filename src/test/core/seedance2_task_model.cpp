//
// seedance2 任务数据模型测试
//
// 盯的是推理任务在数据模型上的三处约定, 它们都属于「改错了不会立刻报错, 但会让历史数据读不出来
// 或者账目错乱」的那一类:
//
//   1) task_type 在库里以 magic_enum 的**名字串**存储, 所以枚举名不能改、只能追加,
//      而且 JSON 名必须与 magic_enum 推出来的名字**逐字一致**(否则写进去的行自己读不出来);
//   2) 推理任务的 ai_light_iteration_id / result_text 要能 JSON 往返, 且旧数据(没有这两个
//      字段)必须仍然可解析 —— 它们对旧客户端是选填;
//   3) 推理任务的 token 口径是 total_tokens, 任务默认值(g_max_task_completion_tokens)是给
//      生成类任务用的上限, 两者不能搞混。
//

#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(seedance2_task_model)

namespace {
using namespace doodle;

const std::string g_iteration{"1a2b3c4d-5e6f-4a1b-8c2d-3e4f5a6b7c8d"};

nlohmann::json minimal_task_json() {
  // from_json 里的必填项: data_request / ai_studio_id / project_uuid_id / type / backend /
  // ai_generate_entity_id
  return nlohmann::json{
      {"data_request", nlohmann::json{{"model", "deepseek-v4-1-flash-260910"}}},
      {"ai_studio_id", "00000000-0000-0000-0000-000000000000"},
      {"project_uuid_id", "00000000-0000-0000-0000-000000000000"},
      {"ai_generate_entity_id", "00000000-0000-0000-0000-000000000000"},
      {"type", "inference"},
      {"backend", "seedance2"},
  };
}
}  // namespace

// ---------------------------------------------------------------------------
// task_type: 名字串就是库里的存储形式
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(task_type_names_are_stable_and_append_only) {
  // 这两个名字出现在**已经在库里的行**里, 改名等于让历史数据读不出来
  BOOST_CHECK_EQUAL(std::string{magic_enum::enum_name(doodle::seedance2::task_type::picture)}, std::string{"picture"});
  BOOST_CHECK_EQUAL(std::string{magic_enum::enum_name(doodle::seedance2::task_type::video)}, std::string{"video"});
  // 新值只能追加在末尾
  BOOST_CHECK_EQUAL(
      std::string{magic_enum::enum_name(doodle::seedance2::task_type::inference)}, std::string{"inference"}
  );
  BOOST_CHECK_EQUAL(static_cast<int>(doodle::seedance2::task_type::picture), 0);
  BOOST_CHECK_EQUAL(static_cast<int>(doodle::seedance2::task_type::video), 1);
  BOOST_CHECK_EQUAL(static_cast<int>(doodle::seedance2::task_type::inference), 2);
}

BOOST_AUTO_TEST_CASE(task_type_json_name_matches_magic_enum_name) {
  // 这是最关键的一致性: 落库用 magic_enum 的名字, 前后端交互用 JSON 的名字,
  // 两者一旦不一致, 写进去的行就读不回来了。
  //
  // 注意这里一律用**括号** nlohmann::json(x): 花括号会走 initializer_list 构造,
  // 把值塞进一个单元素数组(见 §15.4 坑 1), 于是 get<std::string>() 抛 302。
  BOOST_CHECK_EQUAL(
      std::string{magic_enum::enum_name(doodle::seedance2::task_type::inference)}, std::string{"inference"}
  );
  BOOST_CHECK_EQUAL(nlohmann::json(doodle::seedance2::task_type::inference).get<std::string>(), "inference");

  for (const auto l_type :
       {doodle::seedance2::task_type::picture, doodle::seedance2::task_type::video,
        doodle::seedance2::task_type::inference}) {
    const auto l_name = std::string{magic_enum::enum_name(l_type)};
    BOOST_CHECK_EQUAL(nlohmann::json(l_type).get<std::string>(), l_name);
    // 反向: JSON 名 -> 枚举 -> JSON 名
    BOOST_CHECK_EQUAL(
        nlohmann::json(nlohmann::json(l_name).get<doodle::seedance2::task_type>()).get<std::string>(), l_name
    );
  }
}

BOOST_AUTO_TEST_CASE(unknown_type_is_rejected_instead_of_becoming_picture) {
  // nlohmann 的 SERIALIZE_ENUM 对认不出的字符串**不抛**, 而是静默落到第一个枚举值
  // (nlohmann/detail/macro_scope.hpp:247)。若不显式校验, 一个拼错的 type 会变成
  // 图片生成任务 —— 这是本轮加 checked_enum_from_json() 的直接原因。
  for (const auto* l_bad : {"infer", "Inference", "image", ""}) {
    auto l_json    = minimal_task_json();
    l_json["type"] = l_bad;
    BOOST_CHECK_THROW(l_json.get<doodle::seedance2::task>(), nlohmann::json::type_error);
  }

  // backend 同理
  auto l_json       = minimal_task_json();
  l_json["backend"] = "volcano";
  BOOST_CHECK_THROW(l_json.get<doodle::seedance2::task>(), nlohmann::json::type_error);

  // 合法值仍然正常(校验不能把正常路径也挡了)
  BOOST_CHECK_EQUAL(
      minimal_task_json().get<doodle::seedance2::task>().type_ == doodle::seedance2::task_type::inference, true
  );
}

// ---------------------------------------------------------------------------
// task: 推理字段的 JSON 往返与向后兼容
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(task_json_round_trip_keeps_inference_fields) {
  auto l_json                     = minimal_task_json();
  l_json["ai_light_iteration_id"] = g_iteration;

  const auto l_task               = l_json.get<doodle::seedance2::task>();
  BOOST_CHECK_EQUAL(l_task.type_ == doodle::seedance2::task_type::inference, true);
  BOOST_CHECK_EQUAL(boost::uuids::to_string(l_task.ai_light_iteration_id_), g_iteration);

  // 再写回去: 字段要在(括号而不是花括号, 否则会包成单元素数组)
  const auto l_out = nlohmann::json(l_task);
  BOOST_CHECK_EQUAL(l_out.at("ai_light_iteration_id").get<std::string>(), g_iteration);
  BOOST_CHECK_EQUAL(l_out.at("type").get<std::string>(), "inference");
}

BOOST_AUTO_TEST_CASE(task_from_json_accepts_old_payload_without_inference_fields) {
  // 旧前端/旧行没有这个字段: 必须仍然可解析(它是选填), 而不是抛
  const auto l_task = minimal_task_json().get<doodle::seedance2::task>();

  BOOST_CHECK_EQUAL(l_task.ai_light_iteration_id_.is_nil(), true);
}

BOOST_AUTO_TEST_CASE(task_from_json_ignores_removed_result_text) {
  // result_text 已从 task 删除(推理结果不再单独留档)。带上这个键的旧载荷必须仍然可解析
  // —— from_json 不再读它, 多出来的键不该让解析失败。
  auto l_json           = minimal_task_json();
  l_json["result_text"] = R"({"schema_version": 1})";

  const auto l_task     = l_json.get<doodle::seedance2::task>();
  BOOST_CHECK_EQUAL(l_task.type_ == doodle::seedance2::task_type::inference, true);

  // 也不再写回去 —— 该键从 to_json 的输出里彻底消失
  const auto l_out = nlohmann::json(l_task);
  BOOST_CHECK_EQUAL(l_out.contains("result_text"), false);
}

BOOST_AUTO_TEST_CASE(inference_must_not_reuse_the_generation_token_default) {
  // 生成类任务的默认上限是给异步视频生成预留的; 推理任务按 total_tokens 当场结算, 必须显式赋值
  BOOST_CHECK_EQUAL(doodle::seedance2::task{}.completion_tokens_, doodle_config::g_max_task_completion_tokens);
  BOOST_CHECK_EQUAL(doodle_config::g_ark_inference_default_consumed_tokens > 0, true);
  BOOST_CHECK_EQUAL(
      doodle_config::g_ark_inference_default_consumed_tokens < doodle_config::g_max_task_completion_tokens, true
  );
}

// ---------------------------------------------------------------------------
// person_token: 推理任务的统计桶
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(person_token_exposes_an_inference_counter) {
  doodle::seedance2::person_token l_token{};
  BOOST_CHECK_EQUAL(l_token.inference_count_, 0);
  l_token.inference_count_ = 3;

  const auto l_json        = nlohmann::json(l_token);
  BOOST_CHECK_EQUAL(l_json.at("inference_count").get<std::int64_t>(), 3);
}

BOOST_AUTO_TEST_SUITE_END()
