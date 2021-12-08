//
// Created by TD on 2021/12/8.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <spdlog/sinks/base_sink.h>

#include <maya/MGlobal.h>
namespace doodle::maya_plug {
template <class Mutex>
class maya_msg : public spdlog::sinks::base_sink<Mutex> {
 public:
  maya_msg() = default;

 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    switch (msg.level) {
      case spdlog::level::err: {
        MGlobal::displayError(MString{boost::locale::conv::utf_to_utf<wchar_t>(
                                          fmt::to_string(formatted))
                                          .c_str()});
        break;
      }
      case spdlog::level::warn: {
        MGlobal::displayWarning(MString{boost::locale::conv::utf_to_utf<wchar_t>(
                                            fmt::to_string(formatted))
                                            .c_str()});
        break;
      }
      default: {
        MGlobal::displayInfo(MString{boost::locale::conv::utf_to_utf<wchar_t>(
                                         fmt::to_string(formatted))
                                         .c_str()});
        break;
      }
    }
  }

  void flush_() override {}
};
using maya_msg_mt = maya_msg<std::mutex>;
}  // namespace doodle::maya_plug
