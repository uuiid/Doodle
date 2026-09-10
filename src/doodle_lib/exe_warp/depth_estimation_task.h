//
// Created by TD on 25-7-12.
//
// 深度估计分布式任务 — 在工作机上执行 deep_estimation

#pragma once

#include <doodle_core/metadata/server_task_info.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/work.h>

#include <boost/asio/awaitable.hpp>

#include <filesystem>
#include <memory>

namespace doodle::http {

class depth_estimation_distributed : public base_distributed_task {
  struct args {
    uuid preview_id_;
  };
  args args_;

 public:
  explicit depth_estimation_distributed(server_task_info in_task_info, std::shared_ptr<http_work> in_http_work_ptr)
      : base_distributed_task(std::move(in_task_info), std::move(in_http_work_ptr)) {}
  ~depth_estimation_distributed() override = default;

  boost::asio::awaitable<void> run();
};

}  // namespace doodle::http