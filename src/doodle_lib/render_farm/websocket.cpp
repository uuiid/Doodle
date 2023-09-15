//
// Created by td_main on 2023/9/14.
//

#include "websocket.h"

namespace doodle::render_farm {

void websocket::run() {
  impl_ptr_->stream_.async_read(impl_ptr_->buffer_, [](boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
      return;
    }
  });
}
}  // namespace doodle::render_farm
