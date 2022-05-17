//
// Created by TD on 2022/5/17.
//
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/thread_pool/thread_pool.h>

#include <doodle_core/json_rpc/core/server.h>
#include <doodle_core/json_rpc/json_rpc_client.h>
#include <doodle_core/metadata/move_create.h>

using namespace doodle;
using namespace std::literals;
void test_client() {
  boost::asio::io_context io_context{};
  json_rpc_client l_c{io_context, "127.0.0.1"s, std::uint16_t{10223}};
  auto l_prj = l_c.open_project("D:/");
  std::cout << l_prj.p_path << std::endl;

  std::vector<movie::image_attr> l_vector{};

  boost::coroutines2::coroutine<json_rpc::args::rpc_json_progress>::pull_type
      l_push_type{[&](boost::coroutines2::coroutine<json_rpc::args::rpc_json_progress>::push_type& in_skin) {
        l_c.image_to_move(in_skin, l_vector);
      }};
  for (auto&& i : l_push_type) {
    std::cout << i.msg_ << std::endl;
  }
}

TEST_CASE("test json rpc") {
  auto l_app = app{};
  g_thread_pool().enqueue([]() {
    json_rpc::server l_server{g_io_context(), 10223};
    g_io_context().run();
  });
  g_main_loop().attach<one_process_t>([]() {
    test_client();
  });
  l_app.run();
}
