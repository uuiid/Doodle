//
// Created by TD on 2021/6/17.
//

#pragma once
#include <string>
#include <array>

namespace doodle::doodle_config {
constexpr const std::string_view doodle_db_name{".doodle_db"};
constexpr const std::string_view doodle_json_extension{".json_doodle"};
constexpr const std::string_view token_name{"token.doodle_token"};
constexpr const std::string_view authorization_data{"doodle"};
constexpr const std::array<unsigned char, 32> cryptopp_key{"cryptopp_key.uuiid.doodle.v.3.1"};
constexpr const std::array<unsigned char, 16> cryptopp_iv{"iv.uuiid.doodle"};
constexpr const std::int16_t cryptopp_tag_size{16};
constexpr const std::string_view drop_imgui_id{"drop_imgui_id"};
constexpr const std::string_view drop_handle_list{"drop_handle_list"};
constexpr const std::size_t rand_block{256};
constexpr const std::string_view font_default{R"(C:/Windows/Fonts/simkai.ttf)"};
constexpr const std::string_view image_folder_name{R"(image)"};

constexpr const std::string_view ue_path_obj{"Engine/Binaries/Win64/UE4Editor.exe"};
constexpr const std::string_view ue4_content{R"(Content)"};
constexpr const std::string_view ue4_game{R"(/Game)"};
constexpr const std::string_view ue4_shot{R"(Shot)"};
}  // namespace doodle::doodle_config
