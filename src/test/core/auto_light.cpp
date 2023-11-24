//
// Created by TD on 2023/11/23.
//

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/maya_to_exe_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>

constexpr auto l_data = R"([
{
    "out_file": "D:/test_files/test_anim_11_20/fbx/LQ_ep092_sc089/LQ_ep092_sc089_Ch426A_rig_wxc_1001-1013.fbx",
    "ref_file": "C:/sy/LianQiShiWanNian_8/6-moxing/Ch/JDCh_05/Ch426A/Rig/Ch426A_rig_wxc.ma"
},
{
    "out_file": "",
    "ref_file": "C:/sy/LianQiShiWanNian_8/6-moxing/BG/JD_05/BG027C/Mod/YuDaoZong_TingYuan_Low.ma"
},
{
    "out_file": "D:/test_files/test_anim_11_20/fbx/LQ_ep092_sc089/LQ_ep092_sc089_camera_1001-1013.fbx",
    "ref_file": ""
}
])";

class maya_exe_test : public doodle::maya_exe {
 public:
  maya_exe_test()           = default;
  ~maya_exe_test() override = default;

 private:
  void queue_up(
      const entt::handle& in_msg, const std::string_view& in_key, const nlohmann::json& in_string,
      call_fun_type in_call_fun, const std::filesystem::path& in_run_path
  ) override {
    auto l_path = in_string.get<doodle::maya_exe_ns::export_fbx_arg>().out_path_file_;
    doodle::FSys::ofstream{l_path} << l_data;
    in_call_fun(boost::system::error_code{});  // 通知完成
  }
};

class ue_exe_test : public doodle::ue_exe {
 public:
  ue_exe_test()           = default;
  ~ue_exe_test() override = default;

 private:
  void queue_up(const entt::handle& in_msg, const std::string& in_command_line, call_fun_type in_call_fun) override {
    doodle::log_info(fmt::format("ue_exe_test", in_command_line));
    in_call_fun(boost::system::error_code{});  // 通知完成
  }
};

namespace doodle {

void next_time_run(
    boost::asio::deadline_timer::duration_type in_time,
    boost::asio::any_completion_handler<void(boost::system::error_code)> in_fun
) {
  auto l_time = std::make_shared<boost::asio::deadline_timer>(g_io_context());
  l_time->expires_from_now(in_time);
  l_time->async_wait([l_time, l_fun = std::move(in_fun)](const boost::system::error_code& in_ec) mutable {
    l_fun(in_ec);
  });
}
void test_fun3(boost::system::error_code in_code) {
  auto l_maya_exe = g_ctx().get<maya_exe_ptr>();
  auto k_arg      = maya_exe_ns::export_fbx_arg{};
  const FSys::path l_out_path{"D:/test_files/test_ue_auto_main/test.json"};
  const FSys::path l_file_path{"D:/test_files/test_ue_auto_main/LQ_ep092_sc089.ma"};

  k_arg.file_path        = l_file_path;
  k_arg.out_path_file_   = l_out_path;
  k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
  k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
  auto l_ue_msg          = entt::handle{*g_reg(), g_reg()->create()};
  l_maya_exe->async_run_maya(
      entt::handle{*g_reg(), g_reg()->create()}, k_arg,
      maya_to_exe_file{entt::handle{}, FSys::path{l_out_path}}.set_ue_call_fun(boost::asio::bind_executor(
          g_io_context(),
          [l_ue_msg](boost::system::error_code in_code) {
            auto& l_msg = l_ue_msg.get<process_message>();
            if (in_code) {
              auto l_msg_str = fmt::format("render error:{}", in_code);
              log_error(l_msg_str);
              l_msg.message(l_msg_str);
              l_msg.set_state(l_msg.fail);
              return;
            }
            l_msg.set_name("排屏完成");
            l_msg.set_state(l_msg.success);
          }
      ))
  );
}

void test_fun2(boost::system::error_code in_code) {
  auto l_handle{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle.emplace<database>();
  l_handle.emplace<assets_file>().path_attr("C:/sy/LianQiShiWanNian_8/6-moxing/Ch/JDCh_05/Ch426A/Rig/Ch426A_rig_wxc.ma"
  );
  FSys::software_flag_file(l_handle.get<assets_file>().path_attr(), l_handle.get<database>().uuid());
  log_info(fmt::format("Ch426A_rig_wxc uuid {}", l_handle.get<database>().uuid()));

  auto l_handle_ue{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle_ue.emplace<database>();
  l_handle_ue.emplace<assets_file>().path_attr("C:/sy/LianQiShiWanNian_8/6-moxing/Ch/JDCh_05/Ch426A/BuJiaBan_UE5");
  FSys::software_flag_file(l_handle_ue.get<assets_file>().path_attr(), l_handle_ue.get<database>().uuid());
  log_info(fmt::format("BuJiaBan_UE5 uuid {}", l_handle_ue.get<database>().uuid()));

  auto l_handle2{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle2.emplace<database>();
  l_handle2.emplace<assets_file>().path_attr(
      "C:/sy/LianQiShiWanNian_8/6-moxing/BG/JD_05/BG027C/Mod/YuDaoZong_TingYuan_Low.ma"
  );
  FSys::software_flag_file(l_handle2.get<assets_file>().path_attr(), l_handle2.get<database>().uuid());
  log_info(fmt::format("YuDaoZong_TingYuan_Low uuid {}", l_handle2.get<database>().uuid()));

  auto l_handle_ue2{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle_ue2.emplace<database>();
  l_handle_ue2.emplace<assets_file>().path_attr("C:/sy/LianQiShiWanNian_8/6-moxing/BG/JD_05/BG027C/YuDaoZong_TingYuan");

  l_handle_ue2.emplace<ue_main_map>(R"(/Game/YuDaoZong_TingYuan/Map/YuDaoZong_TingYuan)");
  FSys::software_flag_file(l_handle_ue2.get<assets_file>().path_attr(), l_handle_ue2.get<database>().uuid());
  log_info(fmt::format("YuDaoZong_TingYuan uuid {}", l_handle_ue2.get<database>().uuid()));

  auto l_handle3{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle3.emplace<database>();
  l_handle3.emplace<file_association>().maya_file = l_handle;
  l_handle3.patch<file_association>().ue_file     = l_handle_ue;

  auto l_handle4{entt::handle{*g_reg(), g_reg()->create()}};
  l_handle4.emplace<database>();
  l_handle4.emplace<file_association>().maya_file = l_handle2;
  l_handle4.patch<file_association>().ue_file     = l_handle_ue2;

  l_handle.emplace<file_association_ref>(l_handle3);
  l_handle_ue.emplace<file_association_ref>(l_handle3);

  l_handle2.emplace<file_association_ref>(l_handle4);
  l_handle_ue2.emplace<file_association_ref>(l_handle4);
  doodle::test_fun3({});
}

void test_fun(boost::system::error_code in_code) {
  g_ctx().emplace<maya_exe_ptr>() = std::make_shared<maya_exe_test>();
  g_ctx().emplace<ue_exe_ptr>()   = std::make_shared<ue_exe_test>();

  g_ctx().emplace<database_n::file_translator_ptr>()->set_only_open(true).async_open("E:/cache/doodle_main2.doodle_db");
  next_time_run({0, 0, 1}, doodle::test_fun2);
}
}  // namespace doodle

int core_auto_light(int argc, char* argv[]) {
  using main_app = doodle::app_command<>;
  main_app l_app{argc, argv};
  doodle::next_time_run({0, 0, 1}, doodle::test_fun);

  return l_app.run();
}
