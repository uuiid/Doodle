//
// Created by td_main on 2023/9/4.
//

#pragma once
namespace doodle {

class udp_client {
 public:
  udp_client()  = default;
  ~udp_client() = default;

  void async_find_server();
};

}  // namespace doodle
