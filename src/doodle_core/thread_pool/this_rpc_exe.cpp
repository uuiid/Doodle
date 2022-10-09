//
// Created by TD on 2022/10/9.
//

#include "this_rpc_exe.h"

#include <boost/process.hpp>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/json_rpc/json_rpc_client.h>
#include <doodle_core/platform/win/get_prot.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
namespace doodle::detail {
class this_rpc_exe::impl {
 public:
  FSys::path this_exe_path;

  boost::process::child this_exe_proces;

  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};
  boost::asio::streambuf out_io_out_attr;
  boost::asio::streambuf out_io_err_attr;
  ::doodle::process_message* msg;
  //  std::shared_ptr<json_rpc_client> rpc_child;
};

this_rpc_exe::this_rpc_exe()
    : ptr(std::make_unique<impl>()) {
  //  ptr->this_exe_path = core_set::get_set().program_location() / "DoodleExe.exe"s;
}
void this_rpc_exe::stop_exit() {
  //  if (ptr->this_exe_proces.valid() && ptr->rpc_child) {
  //    ptr->rpc_child->stop_app();
  //  }
}
void this_rpc_exe::create_move(
    const FSys::path& in_out_path,
    const std::vector<doodle::movie::image_attr>& in_move,
    ::doodle::process_message& in_msg
) {
  nlohmann::json l_json{};
  auto l_h = make_handle();
  entt_tool::save_comm<episodes, shot, FSys::path, std::vector<doodle::movie::image_attr>>(
      l_h,
      l_json
  );
  auto l_tmp = FSys::write_tmp_file(
      "create_move",
      l_json.dump(), ".json"
  );
  ptr->this_exe_proces = boost::process::child{
      boost::process::exe  = ptr->this_exe_path,
      boost::process::args = fmt::format(R"(--json_rpc --create_move="{}")", l_tmp),
      boost::process::std_out > ptr->out_attr,
      boost::process::std_err > ptr->err_attr};
  
  this->read_err();
  this->read_out();

  //  ptr->rpc_child->create_movie({in_out_path, in_move});
  //  in_msg.message();
}
void this_rpc_exe::create_rpc_child() {
  //  if (ptr->this_exe_proces.valid() && ptr->rpc_child)
  //    return;

  //  ptr->this_exe_proces = boost::process::child{
  //      boost::process::exe  = ptr->this_exe_path,
  //      boost::process::args = "--json_rpc"};
  //
  //  ptr->rpc_child = std::make_shared<json_rpc_client>(
  //      g_io_context(),
  //      "127.0.0.1"s,
  //      win::get_tcp_port(ptr->this_exe_proces.id())
  //  );
}
void this_rpc_exe::read_err() const {
  boost::asio::async_read_until(
      ptr->out_attr,
      ptr->out_io_err_attr,
      '\n',
      [this](boost::system::error_code in_code, std::size_t in_size) {
        if (in_code) {
          std::istream l_istream{&ptr->out_io_err_attr};
          std::string l_ine{};
          std::getline(l_istream, l_ine);
          if (l_ine.empty())
            return;
          if (ptr->msg) {
            ptr->msg->message(l_ine);
          }
          this->read_err();
        } else
          DOODLE_LOG_INFO("错误 {}", in_code.message());
      }
  );
}
void this_rpc_exe::read_out() const {
  boost::asio::async_read_until(
      ptr->out_attr,
      ptr->out_io_out_attr,
      '\n',
      [this](boost::system::error_code in_code, std::size_t in_size) {
        if (in_code) {
          std::istream l_istream{&ptr->out_io_out_attr};
          std::string l_ine{};
          std::getline(l_istream, l_ine);
          if (l_ine.empty())
            return;
          if (ptr->msg) {
            ptr->msg->message(l_ine);
          }
          this->read_out();
        } else
          DOODLE_LOG_INFO("错误 {}", in_code.message());
      }
  );
}

this_rpc_exe::~this_rpc_exe() = default;
}  // namespace doodle::detail
