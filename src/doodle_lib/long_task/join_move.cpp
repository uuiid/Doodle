//
// Created by TD on 2021/12/27.
//

#include "join_move.h"

#include <doodle_core/core/util.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/thread_pool/process_message.h>

#include <opencv2/opencv.hpp>

namespace doodle {
namespace details {

class join_move::impl {
 public:
  entt::handle handle_;

  FSys::path out_path_;
  std::vector<FSys::path> in_list;
  std::future<void> future_;
  std::atomic_bool stop_{false};
};
join_move::join_move(const entt::handle &in_handle, const std::vector<FSys::path> &in_vector)
    : p_i(std::make_unique<impl>()) {
  if (!in_handle.all_of<FSys::path, process_message>())
    throw_exception(doodle_error{"缺失组件"}, BOOST_CURRENT_LOCATION);
  p_i->handle_   = in_handle;
  p_i->out_path_ = in_handle.get<FSys::path>();
  p_i->in_list   = in_vector | ranges::views::filter([](const FSys::path &in_path) {
                   return in_path.extension() == ".mp4" || in_path.extension() == ".avi";
                 }) |
                 ranges::to_vector;
}
void join_move::link_move() {
  /// @brief 进行输入文件的排序排序

  p_i->in_list |=
      ranges::actions::sort([](const FSys::path &k_l, const FSys::path &k_r) { return k_l.stem() < k_r.stem(); });

  /// @brief 创建输出目录
  if (exists(p_i->out_path_.parent_path())) create_directories(p_i->out_path_.parent_path());

  auto k_video_input = cv::VideoCapture{};
  const static cv::Size k_size{1920, 1080};
  auto k_video_out =
      cv::VideoWriter{p_i->out_path_.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 25, k_size};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  const auto k_len     = p_i->in_list.size();

  for (const auto &path : p_i->in_list) {
    if (p_i->stop_) return;
    DOODLE_CHICK(k_video_input.open(path.generic_string()), doodle_error{"文件 {} 的格式不支持", path});
    // 获得总帧数
    auto k_frame_count =
        boost::numeric_cast<std::size_t>(k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT));

    while (k_video_input.read(k_image)) {
      if (p_i->stop_) return;
      //      std::size_t k_frame = k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES);
      if (k_image.cols != k_size.width || k_image.rows != k_size.height)
        cv::resize(k_image, k_image_resized, k_size);
      else
        k_image_resized = k_image;

      k_video_out << k_image_resized;
      p_i->handle_.get<process_message>().progress_step(rational_int{1, k_frame_count * k_len});
    }
  }
}
void join_move::init() {
  if (!p_i->out_path_.has_extension()) {
    p_i->out_path_ /=
        (p_i->handle_.all_of<episodes>() ? fmt::to_string(p_i->handle_.get<episodes>())
                                         : fmt::format("move_{}", ::doodle::core::identifier::get().id()));
    p_i->out_path_ += ".mp4";
  } else if (p_i->out_path_.extension() != ".mp4")
    p_i->out_path_.replace_extension(".mp4");

  auto &l_msg = p_i->handle_.get<process_message>();
  l_msg.set_name(p_i->out_path_.filename().generic_string());
  l_msg.set_state(l_msg.run);
  l_msg.aborted_function = [this]() { this->abort(); };
  //  p_i->future_           =
}
void join_move::succeeded() {
  auto &l_msg = p_i->handle_.get<process_message>();
  l_msg.message(fmt::format("完成视频 {} ", p_i->out_path_));

  l_msg.set_state(l_msg.success);
}
void join_move::failed() {
  auto &l_msg = p_i->handle_.get<process_message>();
  l_msg.message(fmt::format("视频 {} 创建失败", p_i->out_path_));
  l_msg.set_state(l_msg.fail);
}
void join_move::aborted() {
  p_i->stop_  = true;
  auto &l_msg = p_i->handle_.get<process_message>();
  l_msg.message("用户主动结束任务"s);
  l_msg.set_state(l_msg.fail);
}
void join_move::update(const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &, void *data) {
  switch (p_i->future_.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->future_.get();
        this->succeed();
      } catch (const doodle_error &error) {
        DOODLE_LOG_ERROR(boost::diagnostic_information(error));
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}

join_move::~join_move() = default;

}  // namespace details
}  // namespace doodle
