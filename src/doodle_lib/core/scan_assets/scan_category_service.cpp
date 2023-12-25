//
// Created by TD on 2023/12/23.
//

#include "scan_category_service.h"
namespace doodle::details {
namespace {
template <class Mutex>
class scan_sink_t : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<scan_category_service_t::logger_data_t> logger_data_;

 public:
  explicit scan_sink_t(std::shared_ptr<scan_category_service_t::logger_data_t> in_logger_data_)
      : logger_data_{std::move(in_logger_data_)} {}

 private:
 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // 格式化
    std::lock_guard const _lock{logger_data_->mutex_};
    logger_data_->data_.append(msg.payload.data(), msg.payload.size());
    logger_data_->data_.append("\n");
  }
  void flush_() override{};
};
}  // namespace
void scan_category_service_t::init_logger_data() {
  logger_data_ = std::make_shared<logger_data_t>();
  auto l_sink  = std::make_shared<scan_sink_t<std::mutex>>(logger_data_);
  logger_      = std::make_shared<spdlog::logger>("scan_category", l_sink);
  logger_->sinks().emplace_back(g_logger_ctrl().rotating_file_sink_);
}

}  // namespace doodle::details