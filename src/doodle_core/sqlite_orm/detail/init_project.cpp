//
// Created by TD on 24-9-23.
//

#include "init_project.h"

#include <doodle_core/sqlite_orm/sqlite_database.h>

namespace doodle::details {

namespace {
auto create_prj() {
  return std::vector{
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "独步逍遥",
          .path_             = R"(//192.168.10.250/public/DuBuXiaoYao_3)",
          .en_str_           = "DuBuXiaoYao",
          .shor_str_         = "DB",
          .local_path_       = "V:/",
          .auto_upload_path_ = R"(\\192.168.10.250\public\HouQi\1-DuBuXiaoYao\1_DBXY_TiJiaoWenJian\)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "万古邪帝",
          .path_             = R"(//192.168.10.240/public/WGXD)",
          .en_str_           = "WanGuXieDi",
          .shor_str_         = "DW",
          .local_path_       = "C:/sy/WGXD",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\WanGuXieDi\)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "炼气十万年",
          .path_             = R"(//192.168.10.240/public/LianQiShiWanNian)",
          .en_str_           = "LianQiShiWanNian",
          .shor_str_         = "LQ",
          .local_path_       = R"(C:\sy\LianQiShiWanNian_8)",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\LianQiShiWanNian\)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "万古神话",
          .path_             = R"(//192.168.10.240/public/WanGuShenHua)",
          .en_str_           = "WanGuShenHua",
          .shor_str_         = "WG",
          .local_path_       = "R:/",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\WanGuShenHua\)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "无尽神域",
          .path_             = R"(//192.168.10.240/public/WuJinShenYu)",
          .en_str_           = "WuJinShenYu",
          .shor_str_         = "WJ",
          .local_path_       = R"(C:\sy\WuJinShenYu_8)",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\WuJinShenYu)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "宗门里除了我都是卧底",
          .path_             = R"(//192.168.10.240/public/ZMLCLWDSWD)",
          .en_str_           = "ZMLCLWDSWD",
          .shor_str_         = "ZM",
          .local_path_       = R"(C:\sy\ZMLCLWDSWD)",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\ZMLCLWDSWD\)"
      }),
      std::make_shared<project_helper::database_t>(project_helper::database_t{
          .uuid_id_          = core_set::get_set().get_uuid(),
          .name_             = "我的师兄太强了",
          .path_             = R"(//192.168.10.240/public/WDSXTQL)",
          .en_str_           = "WDSXTQL",
          .shor_str_         = "WD",
          .local_path_       = R"(C:\sy\WDSXTQL)",
          .auto_upload_path_ = R"(\\192.168.10.240\public\后期\WDSXTQL\)"
      }),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
      std::make_shared<project_helper::database_t>(),
  };
}
}  // namespace

void init_project() {
  auto l_prjs = g_ctx().get<sqlite_database>().get_all<project_helper::database_t>();
  if (l_prjs.empty()) {
    boost::asio::co_spawn(
        g_io_context(),
        []() -> boost::asio::awaitable<void> {
          auto& l_data = g_ctx().get<sqlite_database>();
          for (auto l_project : create_prj()) {
            co_await l_data.install(l_project);
          }
        },
        boost::asio::detached
    );
  }
}

}  // namespace doodle::details