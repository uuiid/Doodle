//
// Created by TD on 2023/12/23.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/scan_assets/base.h>

#include <boost/asio.hpp>
#include <boost/asio/prepend.hpp>
namespace doodle::details {

class scan_category_service_t {
 public:
 private:
  void init_logger_data();
  boost::asio::thread_pool thread_pool_{};

 public:
  logger_ptr logger_;

  scan_category_service_t() : logger_{} { init_logger_data(); }

  virtual ~scan_category_service_t() = default;
  template <typename CompletionHandler>
  auto async_scan_files(
      const scan_category_data_t::project_root_t& in_project_root,
      const std::shared_ptr<scan_category_t>& in_scan_category_ptr, CompletionHandler&& in_completion
  ) {
    boost::ignore_unused(this);
    in_scan_category_ptr->logger_ = logger_;
    return boost::asio::async_initiate<
        CompletionHandler, void(std::vector<scan_category_data_ptr>, boost::system::error_code)>(
        [&in_project_root, &in_scan_category_ptr, this](auto&& in_completion_handler) {
          auto l_f = std::make_shared<std::decay_t<decltype(in_completion_handler)> >(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          boost::asio::post(thread_pool_, [in_project_root, in_scan_category_ptr, l_f]() {
            boost::system::error_code l_err{};
            std::vector<scan_category_data_ptr> l_list;
            try {
              l_list = in_scan_category_ptr->scan(in_project_root);
            } catch (const FSys::filesystem_error& e) {
              in_scan_category_ptr->logger_->log(log_loc(), level::err, e.what());
              l_err = e.code();
            } catch (...) {
              l_err = boost::system::errc::make_error_code(boost::system::errc::not_supported);
            }
            boost::asio::post(boost::asio::prepend(std::  move(*l_f), l_list, l_err));
          });
        },
        in_completion
    );
  };
};

}  // namespace doodle::details