//
// Created by TD on 2022/6/1.
//

#pragma once

#define DOODLE_SQLITE_TYPE_(...) __VA_ARGS__

#define DOODLE_SQLITE_TYPE DOODLE_SQLITE_TYPE_(doodle::project, doodle::episodes, doodle::shot, doodle::season, doodle::assets, doodle::assets_file, doodle::time_point_wrap, doodle::comment, doodle::image_icon, doodle::importance, doodle::organization_list, doodle::redirection_path_info, doodle::business::rules, doodle::user)

#define DOODLE_SQLITE_TYPE_CTX DOODLE_SQLITE_TYPE_(doodle::project, doodle::project_config::base_config)
