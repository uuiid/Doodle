#include "ai_client_base.h"

namespace doodle::http::seedance2 {
boost::asio::awaitable<void> ai_client_base::query_task_result_t::download() {
  DOODLE_CHICK(client_ptr_, "client_ptr_ is null");
  return client_ptr_->download_result(this);
}
}  // namespace doodle::http::seedance2