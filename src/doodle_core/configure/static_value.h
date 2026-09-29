//
// Created by TD on 2021/6/17.
//

#pragma once
#include <array>
#include <chrono>
#include <string_view>

namespace doodle {
namespace doodle_config {
constexpr std::string_view doodle_db_name{R"(.doodle_db)"};
constexpr std::string_view doodle_flag_name{R"(.doodle_flag)"};
constexpr std::string_view config_name{R"(doodle_config_v3)"};
constexpr std::string_view doodle_token_name{R"(doodle_token)"};

constexpr std::string_view doodle_json_extension{".json_doodle"};
constexpr std::string_view token_name{"token.doodle_token"};
constexpr std::string_view authorization_data{"doodle"};
constexpr std::array<unsigned char, 32> cryptopp_key{"cryptopp_key.uuiid.doodle.v.3.1"};
constexpr std::array<unsigned char, 16> cryptopp_iv{"iv.uuiid.doodle"};
constexpr std::int16_t cryptopp_tag_size{16};
constexpr std::string_view drop_imgui_id{"drop_imgui_files"};
constexpr std::string_view drop_handle_list{"drop_handle_list"};
constexpr std::string_view drop_ass_tree_node{"drop_ass_tree_node"};
constexpr std::size_t rand_block{256};
constexpr std::string_view font_default{R"(C:/Windows/Fonts/simkai.ttf)"};
constexpr std::string_view image_folder_name{R"(image)"};

constexpr std::string_view ue_path_obj{"Engine/Binaries/Win64/UnrealEditor-Cmd.exe"};
constexpr std::string_view ue4_content{R"(Content)"};
constexpr std::string_view ue4_config{R"(Config)"};
constexpr std::string_view ue4_uproject_ext{R"(.uproject)"};
constexpr std::string_view ue4_uasset_ext{R"(.uasset)"};
constexpr std::string_view ue4_umap_ext{R"(.umap)"};

constexpr std::string_view ue4_saved{R"(Saved)"};
constexpr std::string_view ue4_movie_renders{R"(MovieRenders)"};
constexpr std::string_view ue4_game{R"(/Game)"};
constexpr std::string_view ue4_prop{R"(Prop)"};

constexpr std::string_view ue4_shot{R"(Shot)"};
constexpr std::string_view hello_world_doodle{R"(hello world! doodle)"};                // 客户端发送
constexpr std::string_view hello_world_doodle_server{R"(hello world! doodle server)"};  // 服务器发送
constexpr std::string_view g_cache_path{"D:/doodle/cache/ue"};
constexpr std::uint16_t udp_port{50022};
constexpr std::uint16_t http_port{50021};
constexpr std::string_view task_status_todo{"Todo"};
constexpr std::string_view entity_type_sequence{"Sequence"};
constexpr std::string_view entity_type_episode{"Episode"};
constexpr std::string_view entity_type_shot{"Shot"};
constexpr auto g_timeout{std::chrono::seconds(30)};
constexpr auto g_max_keep_alive{std::chrono::seconds(60)};
namespace work_websocket_event {
constexpr std::string_view post_task{R"(post_task)"};
constexpr std::string_view list_task{R"(get_task)"};
}  // namespace work_websocket_event
namespace server_websocket_event {
constexpr std::string_view set_state{R"(set_state)"};
constexpr std::string_view logger{R"(logger)"};
constexpr std::string_view set_task_state{R"(set_task_state)"};
}  // namespace server_websocket_event

constexpr std::int64_t g_max_task_completion_tokens{50'0000};                        // 一个任务预先消耗的最大token数量
constexpr std::int64_t g_max_completion_tokens{g_max_task_completion_tokens * 100};  // 当周任务消耗的最大token数量

// ─── 火山方舟 Responses API(同步推理) ────────────────────────────────────────
// 见 src/doodle_lib/http_client/seedance2_client.h 的 build_inference_body()
constexpr std::string_view g_ark_inference_model{"deepseek-v4-1-flash-260910"};  // 请求未指定 model 时的缺省
constexpr auto g_ark_inference_timeout{std::chrono::seconds(180)};               // 单次推理的读取超时
constexpr std::size_t g_ark_inference_max_retry{3};                              // 429/5xx 的最大尝试次数
constexpr std::int64_t g_ark_inference_default_max_output_tokens{4096};          // 缺省最大输出 token
// 推理任务的预扣上限; 真正计费按响应里的 total_tokens(输入占大头), 差额会退补。
// 推理任务通常是服务端同步结算(插入即终态), 这个值主要用于「按标准提交路径」时的配额检查。
constexpr std::int64_t g_ark_inference_default_consumed_tokens{50'000};
}  // namespace doodle_config
}  // namespace doodle
