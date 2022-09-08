//
// Created by TD on 2022/5/17.
//

#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

class DOODLE_CORE_API json_rpc_process_t : public process_t<json_rpc_process_t> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  json_rpc_process_t(std::uint16_t in_port);
  ~json_rpc_process_t();
  void init();
  void succeeded();
  void failed();
  void aborted();
  void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data
  );
};

}  // namespace doodle
