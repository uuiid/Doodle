//
// Created by TD on 2022/10/9.
//

#include "this_rpc_exe.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/platform/win/get_prot.h>

#include <boost/asio.hpp>
#include <boost/process.hpp>

#include "app/this_rpc_exe.h"

namespace doodle::detail {
class this_rpc_exe::impl {
 public:
  FSys::path this_exe_path{};

  boost::process::child this_exe_proces{};

  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};
  std::string out_io_out_attr{};
  std::string out_io_err_attr{};
  ::doodle::process_message* msg{};
  //  std::shared_ptr<json_rpc_client> rpc_child;
};

this_rpc_exe::this_rpc_exe() : ptr(std::make_unique<impl>()) {}
void this_rpc_exe::stop_exit() {
  //  if (ptr->this_exe_proces.valid() && ptr->rpc_child) {
  //    ptr->rpc_child->stop_app();
  //  }
}
void this_rpc_exe::create_move(
    const FSys::path& in_out_path, const std::vector<doodle::movie::image_attr>& in_move,
    ::doodle::process_message& in_msg
) {
  ptr->this_exe_path = core_set::get_set().program_location() / "DoodleExe.exe"s;
  ptr->msg           = &in_msg;
  nlohmann::json l_json{};
  auto l_h             = make_handle();

  l_json["out_path"]   = in_out_path;
  l_json["image_attr"] = in_move;
  auto l_tmp           = FSys::write_tmp_file("create_move", l_json.dump(), ".json");
  DOODLE_LOG_INFO("开始 doodle 进程 {} ", ptr->this_exe_path);
  ptr->this_exe_proces = boost::process::child{
      boost::process::exe  = ptr->this_exe_path,
      boost::process::args = {"--json_rpc"s, fmt::format(R"(--create_move="{}")", l_tmp)},
      boost::process::std_out > ptr->out_attr, boost::process::std_err > ptr->err_attr};

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
  if (ptr->err_attr.is_open())
    boost::asio::async_read_until(
        ptr->err_attr, boost::asio::dynamic_buffer(ptr->out_io_err_attr), '\n',
        [this](boost::system::error_code in_code, std::size_t in_size) {
          if (!in_code) {
            if (!ptr->out_io_err_attr.empty() && ptr->msg) ptr->msg->message(ptr->out_io_err_attr);
            ptr->out_io_err_attr.clear();
            this->read_err();
          } else
            DOODLE_LOG_INFO("错误 {}", in_code.message());
        }
    );
}
void this_rpc_exe::read_out() const {
  if (ptr->out_attr.is_open())
    boost::asio::async_read_until(
        ptr->out_attr, boost::asio::dynamic_buffer(ptr->out_io_out_attr), '\n',
        [this](boost::system::error_code in_code, std::size_t in_size) {
          if (!in_code) {
            if (!ptr->out_io_out_attr.empty() && ptr->msg) ptr->msg->message(ptr->out_io_out_attr);
            ptr->out_io_out_attr.clear();
            this->read_out();
          } else
            DOODLE_LOG_INFO("错误 {}", in_code.message());
        }
    );
}

this_rpc_exe::~this_rpc_exe() {
  ptr->this_exe_proces.terminate();
  ptr->err_attr.close();
  ptr->out_attr.close();
}
void this_rpc_exe::wait() { ptr->this_exe_proces.wait(); }
}  // namespace doodle::detail
