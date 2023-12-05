//
// Created by TD on 2021/6/17.
//

#pragma once
#include <array>
#include <string>

namespace doodle {
namespace doodle_config {
constexpr const std::string_view doodle_db_name{R"(.doodle_db)"};
constexpr const std::string_view doodle_flag_name{R"(.doodle_flag)"};
constexpr const std::string_view config_name{R"(doodle_config_v2)"};
constexpr const std::string_view doodle_json_extension{".json_doodle"};
constexpr const std::string_view token_name{"token.doodle_token"};
constexpr const std::string_view authorization_data{"doodle"};
constexpr const std::array<unsigned char, 32> cryptopp_key{"cryptopp_key.uuiid.doodle.v.3.1"};
constexpr const std::array<unsigned char, 16> cryptopp_iv{"iv.uuiid.doodle"};
constexpr const std::int16_t cryptopp_tag_size{16};
constexpr const std::string_view drop_imgui_id{"drop_imgui_files"};
constexpr const std::string_view drop_handle_list{"drop_handle_list"};
constexpr const std::string_view drop_ass_tree_node{"drop_ass_tree_node"};
constexpr const std::size_t rand_block{256};
constexpr const std::string_view font_default{R"(C:/Windows/Fonts/simkai.ttf)"};
constexpr const std::string_view image_folder_name{R"(image)"};

constexpr const std::string_view ue_path_obj{"Engine/Binaries/Win64/UnrealEditor-Cmd.exe"};
constexpr const std::string_view ue4_content{R"(Content)"};
constexpr const std::string_view ue4_game{R"(/Game)"};
constexpr const std::string_view ue4_shot{R"(Shot)"};
constexpr const std::string_view hello_world_doodle{R"(hello world! doodle)"};                // 客户端发送
constexpr const std::string_view hello_world_doodle_server{R"(hello world! doodle server)"};  // 服务器发送
constexpr const std::uint16_t udp_port{50022};
constexpr const std::uint16_t http_port{50021};
}  // namespace doodle_config
namespace gui::config::menu_w {
constexpr const std::string_view project_widget{"项目"};
constexpr const std::string_view edit_{"编辑"};
constexpr const std::string_view upload_files{"上传"};
constexpr const std::string_view create_entry_{"创建条目"};
constexpr const std::string_view assets_filter{"过滤"};
constexpr const std::string_view assets_file{"文件列表"};
constexpr const std::string_view long_time_tasks{"队列"};
constexpr const std::string_view setting{"设置窗口"};
constexpr const std::string_view project_edit{"项目设置"};
constexpr const std::string_view xlsx_export{"导出xlsx表格"};
constexpr const std::string_view comm_maya_tool{"maya工具"};
constexpr const std::string_view comm_create_video{"创建视频"};
constexpr const std::string_view extract_subtitles{"提取字幕"};
constexpr const std::string_view subtitle_processing{"修改字幕"};
constexpr const std::string_view time_edit{"时间编辑"};
constexpr const std::string_view all_user_view_widget{"用户列表"};
constexpr const std::string_view render_monitor{"渲染监控"};
constexpr const std::string_view work_hour_filling{"工时填报"};
constexpr const std::string_view database_tool{"库管理工具"};

}  // namespace gui::config::menu_w
}  // namespace doodle
