//
// Created by TD on 25-7-1.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace mailio {
class smtp;
}
namespace doodle::email {
class seed_email {
 private:
  std::string address_;
  std::uint32_t port_;

  std::string username_;
  std::string password_;
  boost::asio::io_context::executor_type executor_;
  std::shared_ptr<mailio::smtp> smtp_;

 public:
  explicit seed_email(std::string in_address, std::uint32_t in_port, std::string in_username, std::string in_password);

  void operator()(const std::string& in_subject, const std::string& in_recipient, const std::string& in_body);

};
}  // namespace doodle::email
