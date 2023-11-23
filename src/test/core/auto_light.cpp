//
// Created by TD on 2023/11/23.
//

#include <doodle_core/database_task/sqlite_client.h>

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
void test_fun() {
  g_ctx().emplace<maya_exe_ptr>() = std::make_shared<maya_exe_test>();
  g_ctx().emplace<ue_exe_ptr>()   = std::make_shared<ue_exe_test>();

  auto l_maya_exe                 = g_ctx().get<maya_exe_ptr>();
  auto k_arg                      = maya_exe_ns::export_fbx_arg{};
  k_arg.file_path                 = "";
  k_arg.out_path_file_            = "";
  k_arg.export_anim_time          = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
  k_arg.project_                  = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
  l_maya_exe->async_run_maya(entt::handle{*g_reg(), g_reg()->create()}, k_arg, maya_to_exe_file{""});
}
}  // namespace doodle

int core_auto_light(int argc, char* argv[]) {
  using main_app = doodle::app_command<>;
  main_app l_app{argc, argv};
  doodle::next_time_run({0, 0, 1}, [](const boost::system::error_code& in_ec) { doodle::test_fun(); });

  return l_app.run();
}
