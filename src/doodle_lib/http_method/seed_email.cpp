//
// Created by TD on 25-7-1.
//

#include "seed_email.h"

#include <mailio/message.hpp>
#include <mailio/smtp.hpp>
namespace doodle::email {

seed_email::seed_email(std::string in_address, std::uint32_t in_port, std::string in_username, std::string in_password)
    : address_(in_address),
      port_(in_port),
      username_(in_username),
      password_(in_password),
      executor_(g_io_context().get_executor()) {}

boost::asio::awaitable<void> seed_email::operator()(
    const std::string& in_subject, const std::string& in_recipient, const std::string& in_body
) {
  DOODLE_TO_EXECUTOR(executor_);
  if (!smtp_) {
    smtp_ = std::make_shared<mailio::smtp>(address_, port_);
    smtp_->authenticate(username_, password_, mailio::smtp::auth_method_t::LOGIN);
  }
  mailio::message l_msg{};
  l_msg.from(mailio::mail_address{"", username_});
  l_msg.add_recipient(mailio::mail_address{"", in_recipient});
  l_msg.subject(in_subject);
  l_msg.content(in_body);

  smtp_->submit(l_msg);
  DOODLE_TO_SELF();
}

}  // namespace doodle::email