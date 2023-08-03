//
// Created by td_main on 2023/8/3.
//
#pragma once
#include <cctype>
#include <string>

namespace doodle {
namespace render_farm {

class server {
 public:
  server()  = default;
  ~server() = default;

  void run(const std::string& in_ip, std::int32_t in_port);

 private:
};

}  // namespace render_farm
}  // namespace doodle
