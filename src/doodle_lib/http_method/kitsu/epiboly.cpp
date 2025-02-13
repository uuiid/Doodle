//
// Created by TD on 24-12-13.
//

#include "epiboly.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> config(session_data_ptr in_handle) {
  co_return in_handle->make_msg(R"(
{
    "is_self_hosted": true,
    "crisp_token": "",
    "dark_theme_by_default": null,
    "indexer_configured": true,
    "saml_enabled": false,
    "saml_idp_name": "",
    "default_locale": "zh",
    "default_timezone": "Asia/Shanghai"
}
)"s);
}
boost::asio::awaitable<boost::beast::http::message_generator> authenticated(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::unauthorized, R"({
    "msg": "Missing JWT in cookies or headers Missing cookie \"access_token_cookie\"; Missing Authorization Header"
})"s);
}

boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  nlohmann::json l_json{};
  l_json["projects"] = nlohmann::json::parse(R"([
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/WDSXTQL/",
    "code": "SX",
    "created_at": "2024-09-12T10:06:46",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "5ef97e7b-980d-48e1-86af-738d68d50c46",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": true,
        "id": "94486f15-fe60-4d7f-bb91-6ca32413bf6f",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "5ef97e7b-980d-48e1-86af-738d68d50c46",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "a05520c6-53d3-48d3-bfa0-fb10b77b6ab1",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "5ef97e7b-980d-48e1-86af-738d68d50c46",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "9639a71f-fb27-4261-845c-a59213656b50",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "f8488f09-1afc-4c0a-ba20-7a2050181504",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "5ef97e7b-980d-48e1-86af-738d68d50c46"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "59d1b087-bcfe-4cd2-bde7-ca6868ad924a",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": true,
        "id": "70d5897e-39a5-4d24-81bb-f584a9e831cc",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "8de84269-37d2-44d1-83bf-0ca376bac254",
        "name": "开始集数"
      }
    ],
    "en_str": "WDSXTQL",
    "end_date": "2026-12-31",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "36e1651f-714a-4b01-b3d7-40a6802ade13",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "我的师兄太强了",
    "nb_episodes": 0,
    "path": "//192.168.10.242/public/WDSXTQL",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2024-05-01",
    "status_automations": [
      "040aa42e-d6a5-4cce-af83-c0a10db0edfb",
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 4,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 2,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 5,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 3,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 7,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 6,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-01-16T10:10:51"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "",
    "code": null,
    "created_at": "2024-12-19T02:09:47",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": true,
        "id": "d11715d7-20f7-4cfa-b5ae-98bc59c917e3",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": true,
        "id": "48cdf1b8-2816-45c8-9cc5-39760e3a44a6",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": true,
        "id": "b64e518a-5c90-4b88-bb97-b4191da130a3",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": false,
        "id": "efbfdf5c-a59e-4ac4-bcf8-19e17cd69ffc",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": true,
        "id": "983cab70-ada4-4e53-85c1-1ca61ee9afc5",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "4f1852a5-28db-46c3-8e89-a834e8ed7a24",
        "name": "开始集数"
      }
    ],
    "en_str": "",
    "end_date": "2031-07-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": 28,
    "homepage": "assets",
    "id": "4f7bdb5a-0efa-4474-9eba-fd43aa69aae0",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": 6,
    "man_days": null,
    "max_retakes": 0,
    "name": "我什么时候无敌了",
    "nb_episodes": 0,
    "path": "",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2024-05-31",
    "status_automations": [],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 3,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 1,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 2,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 4,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-02-12T01:32:55"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/JJ_DJ/",
    "code": "JJ",
    "created_at": "2024-11-07T05:40:42",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": true,
        "id": "920bde6e-359b-4b3d-9dea-29b392df741f",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "cdb59f48-8a41-4e79-8003-bf3545fa40b7",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "50da2c73-c921-418c-87d1-a31e70d6ebc3",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "4503b605-7e82-4751-b0c2-5bbeea290c73",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "d0cb39b7-1144-46dc-b2f5-aadc22780abe",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "42a542ff-75b4-4729-8066-86cbe62e9a27",
        "name": "开始集数"
      }
    ],
    "en_str": "JingJie",
    "end_date": "2027-11-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "d41d3dae-f86b-4080-850d-3d41ef272dd2",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "镜·界",
    "nb_episodes": 0,
    "path": "//192.168.10.242/public/JJ_DJ",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "9:16",
    "resolution": "1080x1920",
    "shotgun_id": null,
    "start_date": "2024-09-01",
    "status_automations": [],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 1,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 2,
      "9be21729-be9e-4914-afc4-6046ed089886": 3,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 2,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-01-10T09:56:23"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/SSWH/",
    "code": null,
    "created_at": "2024-09-24T11:51:32",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": true,
        "id": "30da04c4-bb16-4160-99bc-33d72246984c",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "fa9545ff-64c6-4bd1-b42c-53d87ce79720",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "ac7d3b5a-a409-4cb6-a457-def23a826691",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "b52d2895-8a65-4df9-9812-7ae9582549cd",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "3e7ecd03-ce2d-4a24-871f-fba6bfd2d240",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": true,
        "id": "115a6809-fa3e-412d-a7a3-e36f649e6439",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "884c08a4-3383-4423-8cf2-6e1610b3bb92",
        "name": "开始集数"
      }
    ],
    "en_str": "SSWH",
    "end_date": "2029-12-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "b5b30840-8473-4369-9b5f-c9ee44a9ecdd",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": null,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "双生武魂",
    "nb_episodes": 0,
    "path": "//192.168.10.240/public/SSWH",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2024-01-01",
    "status_automations": [
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 2,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 1,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2024-12-14T09:31:48"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "21b3f5aa-cdd6-4fca-ace4-65077494df4b",
      "8c02b76a-6be6-4959-af58-5c31a85fe072"
    ],
    "auto_upload_path": "//192.168.10.250/public/HouQi/1-DuBuXiaoYao/1_DBXY_TiJiaoWenJian/",
    "code": "DB",
    "created_at": "2024-03-26T05:34:20",
    "data": null,
    "default_preview_background_file_id": null,
    "description": "daaas",
    "descriptors": [
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": true,
        "id": "4fbe15d1-962c-4934-b148-0cbad013b541",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": true,
        "id": "5eb8d2ae-1db5-4df5-aa3b-4ab0a55cc5e2",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": false,
        "id": "30e7f1d3-1e1f-4524-83a3-81687909f703",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu",
        "for_client": false,
        "id": "61935a1c-b120-46fc-b1e6-9fa749913fd2",
        "name": "季数"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": false,
        "id": "29daf566-ef9a-4f39-91fa-ed583c775df1",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "569c259f-129d-44a6-8231-5280f4a69756",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "f484ba08-76ff-441e-9a1f-a8f9b1a76347",
        "name": "开始集数"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "99d0b484-ab3c-4cfb-98d1-a2d88dc3630b",
        "name": "归档"
      }
    ],
    "en_str": "DuBuXiaoYao",
    "end_date": "2027-11-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "eb2b5cde-c1cf-47a0-aba4-e196b6f774dd",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "独步逍遥",
    "nb_episodes": 0,
    "path": "//192.168.10.250/public/DuBuXiaoYao_3",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2023-11-28",
    "status_automations": [
      "040aa42e-d6a5-4cce-af83-c0a10db0edfb",
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "c291b4bf-9cff-4e46-8e01-bbbcc2eabfcb",
      "e37014bd-1d7f-46a3-afad-94dffbafa66e",
      "a393f5f7-97e5-4bba-b77a-43ae9a07066d",
      "dd702215-7284-44ef-b8b0-79b2f4f6e785",
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "02f0636c-1ac5-4bfc-b0f7-70879fc8e8f6",
      "ac9dfe1d-44f6-4903-ade7-efd3e1c55a32",
      "f93092c8-f989-4468-a819-e2d773c01796",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "89a715a3-132c-47e8-a24d-e96a077f48cc",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "a9282404-85fe-4833-9109-28683b73ba85",
      "674e80ed-c5f9-4c21-a52e-d0f9d68f11b1",
      "49fd988c-4c8e-4ba1-8920-085e3cb54359",
      "56ee2fac-4edf-49f5-aced-c9492f584ddb"
    ],
    "task_types_priority": {
      "02f0636c-1ac5-4bfc-b0f7-70879fc8e8f6": null,
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 4,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 5,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 2,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 5,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 3,
      "49fd988c-4c8e-4ba1-8920-085e3cb54359": null,
      "56ee2fac-4edf-49f5-aced-c9492f584ddb": null,
      "674e80ed-c5f9-4c21-a52e-d0f9d68f11b1": null,
      "89a715a3-132c-47e8-a24d-e96a077f48cc": null,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 4,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 3,
      "a393f5f7-97e5-4bba-b77a-43ae9a07066d": null,
      "a9282404-85fe-4833-9109-28683b73ba85": null,
      "ac9dfe1d-44f6-4903-ade7-efd3e1c55a32": null,
      "c291b4bf-9cff-4e46-8e01-bbbcc2eabfcb": null,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 6,
      "dd702215-7284-44ef-b8b0-79b2f4f6e785": null,
      "e37014bd-1d7f-46a3-afad-94dffbafa66e": null,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1,
      "f93092c8-f989-4468-a819-e2d773c01796": null
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-01-17T01:18:44"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/WuJinShenYu",
    "code": null,
    "created_at": "2024-05-11T01:43:15",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": false,
        "id": "e8451bb5-3c70-4deb-87b0-6e3f5c78f17c",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "a47419db-088d-4870-9b65-e7116c9ede3b",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "8d280d0d-23b8-4a8f-9541-2e0a4e696bc2",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "5ef97e7b-980d-48e1-86af-738d68d50c46",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "15695ade-23a3-4c54-b6d6-7916f1ee204d",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": false,
        "id": "3e9e1894-59de-40f7-8a76-205960a0f80d",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [],
        "entity_type": "Shot",
        "field_name": "wu_jin_001ji",
        "for_client": false,
        "id": "9e261e28-2259-4f90-a7cf-64b859cb1da4",
        "name": "无尽001集"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "6c79434c-0a11-4b70-ab4a-860bdd92420e",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "a83d5359-1fdc-4bec-8398-20aa85081094",
        "name": "开始集数"
      }
    ],
    "en_str": "WuJinShenYu",
    "end_date": "2029-03-01",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "d2450560-60d4-451e-aeb8-012d573117cc",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": null,
    "is_set_preview_automated": true,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "无尽神域",
    "nb_episodes": 0,
    "path": "//192.168.10.240/public/WuJinShenYu",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2022-01-01",
    "status_automations": [
      "040aa42e-d6a5-4cce-af83-c0a10db0edfb",
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "e37014bd-1d7f-46a3-afad-94dffbafa66e",
      "a393f5f7-97e5-4bba-b77a-43ae9a07066d",
      "dd702215-7284-44ef-b8b0-79b2f4f6e785",
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "ac9dfe1d-44f6-4903-ade7-efd3e1c55a32",
      "f93092c8-f989-4468-a819-e2d773c01796",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "89a715a3-132c-47e8-a24d-e96a077f48cc",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "a9282404-85fe-4833-9109-28683b73ba85",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029",
      "49fd988c-4c8e-4ba1-8920-085e3cb54359",
      "56ee2fac-4edf-49f5-aced-c9492f584ddb"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 4,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 2,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 5,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 3,
      "49fd988c-4c8e-4ba1-8920-085e3cb54359": null,
      "56ee2fac-4edf-49f5-aced-c9492f584ddb": null,
      "89a715a3-132c-47e8-a24d-e96a077f48cc": null,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "a393f5f7-97e5-4bba-b77a-43ae9a07066d": null,
      "a9282404-85fe-4833-9109-28683b73ba85": null,
      "ac9dfe1d-44f6-4903-ade7-efd3e1c55a32": null,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": null,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 6,
      "dd702215-7284-44ef-b8b0-79b2f4f6e785": null,
      "e37014bd-1d7f-46a3-afad-94dffbafa66e": null,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1,
      "f93092c8-f989-4468-a819-e2d773c01796": null
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-01-08T05:13:34"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.253/public/后期/LianQiShiWanNian/",
    "code": "LQ",
    "created_at": "2024-09-24T02:03:20",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": false,
        "id": "dbb5d6bf-0599-438a-b41b-93d3dd0d8e61",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "d96f45e7-1c52-44d1-b840-3e110c35f143",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "d23a9a27-d2ef-4fa3-bb4e-0ee530321c33",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "fce202be-00f7-40df-addf-b23a660a4aa2",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "dac30e2f-a425-44c7-ab38-c12f15714863",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": true,
        "id": "57023db1-8ebd-4d35-8a55-5eefbb325da9",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": true,
        "id": "51abd4cb-a5bd-4546-bcae-2a09ddb5e9a8",
        "name": "开始集数"
      }
    ],
    "en_str": "LianQiShiWanNian",
    "end_date": "2031-12-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "c340051a-45a6-4af1-a750-efefe639c75b",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "炼气十万年",
    "nb_episodes": 0,
    "path": "//192.168.10.253/public/LianQiShiWanNian",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "short",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2021-12-31",
    "status_automations": [
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 2,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 1,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-02-12T03:27:04"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/ZMLCLWDSWD/",
    "code": "ZM",
    "created_at": "2024-07-02T06:27:21",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": false,
        "id": "211d95cc-a83a-4d67-8223-d8349b8263ea",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "5fd1ea4f-8576-44bd-8fa6-c85b178b5445",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": false,
        "id": "ebc5064b-6213-4f13-ab8f-7d50059124f2",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "521a4a4d-e235-4b6f-b34f-34a2f714ca6d",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "b5ef8ddf-591b-46b5-92cf-bb6a63652961",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "a7c22b9b-7d89-4008-83e3-1002b4559a65"
        ],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": false,
        "id": "db5e3ae9-7543-4732-a5dc-fa79760533f5",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": true,
        "id": "a60be0ea-cedd-4e1d-98d1-d572afbb2f77",
        "name": "开始集数"
      }
    ],
    "en_str": "ZMLCLWDSWD",
    "end_date": "2027-11-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "a2a7cae0-a7ea-4ce9-a26d-2979048bcee0",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": false,
    "is_set_preview_automated": true,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "宗门里除了我都是卧底",
    "nb_episodes": 0,
    "path": "//192.168.10.240/public/ZMLCLWDSWD",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "featurefilm",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2024-12-01",
    "status_automations": [
      "040aa42e-d6a5-4cce-af83-c0a10db0edfb",
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 1,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 2,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2025-01-20T07:51:57"
  },
  {
    "asset_types": [
      "f9a8be37-2d05-4e20-8fae-751a61960ce4",
      "8c02b76a-6be6-4959-af58-5c31a85fe072",
      "6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3",
      "0e40cd9b-7f50-418b-8322-39c451f49dde",
      "6461808e-5fc6-4182-b99a-44b7855249ed",
      "2e869265-f7d6-436e-83aa-516eb7d68eae"
    ],
    "auto_upload_path": "//192.168.10.240/public/后期/WanGuShenHua/",
    "code": null,
    "created_at": "2024-09-24T11:43:47",
    "data": null,
    "default_preview_background_file_id": null,
    "description": null,
    "descriptors": [
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "ji_shu_lie",
        "for_client": true,
        "id": "13715cc9-f1f3-433e-beb5-6488546482de",
        "name": "集数列"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "deng_ji",
        "for_client": false,
        "id": "56ca1135-6a67-4e70-a76c-dbb977eb12b4",
        "name": "等级"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "bian_hao",
        "for_client": true,
        "id": "4f7be6dd-facb-4de7-8fa1-b582d3f76c5b",
        "name": "编号"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "gui_dang",
        "for_client": false,
        "id": "677b0375-6f4b-472d-b23d-8392afe8832a",
        "name": "归档"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "pin_yin_ming_cheng",
        "for_client": false,
        "id": "f2d6a359-ba3e-48dd-918b-2b6092c0e5cb",
        "name": "拼音名称"
      },
      {
        "choices": [],
        "data_type": "string",
        "departments": [],
        "entity_type": "Asset",
        "field_name": "ban_ben",
        "for_client": true,
        "id": "c80fdf79-acab-4b68-afe2-1e548f7f32fa",
        "name": "版本"
      },
      {
        "choices": [],
        "data_type": "number",
        "departments": [
          "0a6ad006-3d02-4c6b-8dd9-51b7c1f83af5",
          "a7c22b9b-7d89-4008-83e3-1002b4559a65",
          "a8b5b53e-b2ec-4571-b01a-59cf53c68949",
          "05d8496f-19d9-4882-ae72-4f54fe9d6e71",
          "cbe4110f-d49e-4079-b364-6ff425a99a44",
          "98deed7d-9795-4cba-a2b6-35b1bf49bcd6"
        ],
        "entity_type": "Asset",
        "field_name": "kai_shi_ji_shu",
        "for_client": false,
        "id": "78227bbc-675e-4939-afdb-20190ccdd78f",
        "name": "开始集数"
      }
    ],
    "en_str": "WanGuShenHua",
    "end_date": "2029-12-30",
    "episode_span": 0,
    "file_tree": null,
    "fps": "25",
    "has_avatar": false,
    "hd_bitrate_compression": null,
    "homepage": "assets",
    "id": "0fc7196c-8fc2-45d5-a60f-822703b370ba",
    "is_clients_isolated": false,
    "is_preview_download_allowed": false,
    "is_publish_default_for_artists": null,
    "is_set_preview_automated": false,
    "ld_bitrate_compression": null,
    "man_days": null,
    "max_retakes": 0,
    "name": "万古神话",
    "nb_episodes": 0,
    "path": "//192.168.10.240/public/WanGuShenHua",
    "preview_background_files": [],
    "production_style": "3d",
    "production_type": "short",
    "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
    "ratio": "16:9",
    "resolution": "1920x1080",
    "shotgun_id": null,
    "start_date": "2023-10-02",
    "status_automations": [
      "040aa42e-d6a5-4cce-af83-c0a10db0edfb",
      "4234451f-5602-4703-be84-6643995f2f74",
      "465d0da5-fe88-4574-ab02-ad260c704377",
      "447d5c42-8aa9-44cd-b30a-3ca11f15c023",
      "a67eb44e-7fd1-4715-8b82-20e72395da9c",
      "0602b443-5488-4d3a-b5dd-63bdfdaf8acd",
      "67a5eebe-f497-4dac-af9c-53431f5667d4",
      "6540ebb4-57d5-4221-b3e5-ca40c31f2273",
      "63ffaabe-928e-4aa4-a9c3-c67491abc1e0",
      "1b9de04d-e874-421c-a979-a7ccb616f830",
      "673aafb2-5cd8-4da7-b97c-640fabc4d7f5"
    ],
    "task_statuses": [
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9",
      "4ffc748e-4e58-4336-ba83-51910253514e",
      "0fae4479-101c-407c-a4c4-88d649e91716",
      "9704a092-76ac-406d-89e5-1dd862e03e82",
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3",
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e"
    ],
    "task_statuses_link": {
      "04458b3f-6287-42e4-93ca-c72a0e5f3bd9": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "0fae4479-101c-407c-a4c4-88d649e91716": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "4ffc748e-4e58-4336-ba83-51910253514e": {
        "priority": null,
        "roles_for_board": [
          "admin",
          "supervisor",
          "manager",
          "vendor",
          "user"
        ]
      },
      "9704a092-76ac-406d-89e5-1dd862e03e82": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b470cd8c-b4e3-4900-84e1-ebaf1439f6e3": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      },
      "b9336b55-fb64-4d9a-a5a3-97fcee6acc1e": {
        "priority": null,
        "roles_for_board": [
          "user",
          "admin",
          "supervisor",
          "manager",
          "vendor"
        ]
      }
    },
    "task_types": [
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2",
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34",
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca",
      "32504e3e-381c-4f36-bdeb-f73328f96f9c",
      "da050d42-4f45-40c4-9638-cc637753d3b5",
      "eb7c92c8-232c-4894-8efa-c62ced44ff05",
      "9be21729-be9e-4914-afc4-6046ed089886",
      "9d71918b-cbf0-46bc-9c39-27177c9a950a",
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f",
      "a33b7371-038c-4628-93b2-6754fc4f302b",
      "c0d5224e-70c3-4b2f-8f49-348d6e051029"
    ],
    "task_types_priority": {
      "13ddf60c-ed8e-4e65-85bb-57dc4207aeca": 3,
      "1a1491e8-5a01-4a7f-bc78-5ee9d2a0aa9f": 4,
      "1a26b082-8cb3-4d3c-9054-50a069ac64b2": 1,
      "32504e3e-381c-4f36-bdeb-f73328f96f9c": 4,
      "3e20ff2b-13e6-4dce-8bf2-37341b5c1f34": 2,
      "9be21729-be9e-4914-afc4-6046ed089886": 2,
      "9d71918b-cbf0-46bc-9c39-27177c9a950a": 3,
      "a33b7371-038c-4628-93b2-6754fc4f302b": 5,
      "c0d5224e-70c3-4b2f-8f49-348d6e051029": 6,
      "da050d42-4f45-40c4-9638-cc637753d3b5": 5,
      "eb7c92c8-232c-4894-8efa-c62ced44ff05": 1
    },
    "team": [],
    "type": "Project",
    "updated_at": "2024-12-06T08:38:36"
  }
])");
  co_return in_handle->make_msg(l_json.dump());
}
}  // namespace
void epiboly_reg(http_route& in_http_route) {
  in_http_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/config", config))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/auth/authenticated", authenticated))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context));
}
}  // namespace doodle::http::kitsu