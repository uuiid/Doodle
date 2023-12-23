//
// Created by TD on 2023/12/23.
//

#pragma once
#include <doodle_lib/core/scan_assets/base.h>

#include <boost/asio/prepend.hpp>
namespace doodle::details {

class scan_category_service_t {
 public:
  scan_category_service_t()          = default;
  virtual ~scan_category_service_t() = default;
  template <typename CompletionHandler>
  auto async_scan_files(
      const scan_category_data_t::project_root_t& in_project_root,
      const std::shared_ptr<scan_category_t>& in_scan_category_ptr, CompletionHandler&& in_completion
  ) {
    boost::ignore_unused(this);
    return boost::asio::async_initiate<
        CompletionHandler, void(std::vector<scan_category_data_ptr>, boost::system::error_code)>(
        [in_project_root, in_scan_category_ptr](auto&& in_completion_handler) {
          auto l_f = std::make_shared<std::decay_t<decltype(in_completion_handler)> >(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          boost::asio::post(g_thread(), [in_project_root, in_scan_category_ptr]() {
            auto l_list = in_scan_category_ptr->scan(in_project_root);
            boost::system::error_code l_err{};
            boost::asio::post(boost::asio::prepend(std::move(*l_f), l_list, l_err));
          });
        },
        in_completion
    );
  };
};

}  // namespace doodle::details