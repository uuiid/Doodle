//
// Created by td_main on 2023/8/11.
//

#pragma once
#include <doodle_server/render_farm/http_session.h>
namespace doodle::render_farm {
// class working_machine_session;
// namespace detail {
// template <typename Body_type>
// class async_read_body_op {
//   using parser_type = boost::beast::http::request_parser<Body_type>;
//  public:
//   template <class Self>
//   void operator()(Self& self, std::shared_ptr<working_machine_session> in_session) {
//     auto l_parser_ptr = std::make_shared<parser_type>(std::move(in_session->request_parser_));
//     boost::beast::http::async_read(
//         in_session->stream_, in_session->buffer_, *l_parser_ptr,
//         [self = in_session, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
//           boost::ignore_unused(bytes_transferred);
//           if (ec == boost::beast::http::error::end_of_stream) {
//             return self->do_close();
//           }
//           if (ec) {
//             DOODLE_LOG_ERROR("on_read error: {}", ec.message());
//             self->send_error_code(ec);
//             return;
//           }
//         }
//     );
//   }
// };
//
// template <typename Body_type, typename CompletionHandler>
// void async_read_body(std::shared_ptr<working_machine_session> in_session, CompletionHandler&& in_handler) {
//   using parser_type = boost::beast::http::request_parser<Body_type>;
//
//   auto l_parser_ptr = std::make_shared<parser_type>(std::move(in_session->request_parser_));
//   boost::beast::http::async_read(
//       in_session->stream_, in_session->buffer_, *l_parser_ptr,
//       [self = in_session, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
//         boost::ignore_unused(bytes_transferred);
//         if (ec == boost::beast::http::error::end_of_stream) {
//           return self->do_close();
//         }
//         if (ec) {
//           DOODLE_LOG_ERROR("on_read error: {}", ec.message());
//           self->send_error_code(ec);
//           return;
//         }
//       }
//   );
// }

//}  // namespace detail
}  // namespace doodle::render_farm