//
// Created by TD on 2021/7/6.
//

#include "actn_image_and_movie.h"

#include <FileWarp/ImageSequence.h>
#include <Gui/action/actn_up_paths.h>
#include <Metadata/Episodes.h>
#include <Metadata/Shot.h>
#include <core/CoreSet.h>

#include <boost/algorithm/string.hpp>

namespace doodle {
actn_image_to_movie::actn_image_to_movie()
    : p_video_path(),
      p_image_sequence() {
  p_name = "转换视频";
}

FSys::path actn_image_to_movie::get_video_path() const {
  return p_video_path;
}

bool actn_image_to_movie::is_accept(const arg_& in_any) {
  const static std::vector<FSys::path> static_path_list{
      ".exr",
      ".jpg",
      ".png"};
  try {
    auto k_path = in_any.image_list.front();
    bool is_ok  = true;
    std::vector<FSys::path> k_r{};
    if (FSys::is_directory(k_path)) {
      FSys::path k_ex{};
      std::copy_if(
          FSys::directory_iterator(k_path), FSys::directory_iterator{},
          std::inserter(k_r, k_r.begin()), [](const FSys::path& in_) {
            if (FSys::is_regular_file(in_))
              return (std::find(static_path_list.begin(), static_path_list.end(), in_.extension()) == static_path_list.end());
            else
              return false;
          });

      is_ok = k_r.empty();
    }

    if (is_ok) {
    }

    return is_ok;
  } catch (const std::bad_cast& err) {
    DOODLE_LOG_WARN("无法转换any ", err.what())
  }
  return false;
}

long_term_ptr actn_image_to_movie::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  p_image_sequence = std::make_shared<ImageSequence>();
  auto k_term = p_image_sequence->get_long_term();

  auto k_path = sig_get_arg().value();
  if (k_path.is_cancel)
    return {};
  auto k_shot = in_parent->find_parent_class<Shot>();
  auto k_eps  = in_parent->find_parent_class<Episodes>();

  auto k_str = CoreSet::getSet().getUser_en();  /// 基本水印, 名称
  /// 如果可以找到集数和镜头号直接添加上去, 否者就这样了
  if (k_shot && k_eps) {
    k_str += fmt::format(" : {}_{}", k_eps->str(), k_shot->str());
  } else if (k_shot) {
    k_str += fmt::format(" : {}", k_shot->str());
  } else if (k_eps) {
    k_str += fmt::format(" : {}", k_eps->str());
  }

  p_image_sequence->setText(k_str);  /// 设置文件水印
  /// 添加文件路径名称
  boost::replace_all(k_str, " ", "_");  /// 替换不好的文件名称组件
  boost::replace_all(k_str, ":", "_");  /// 替换不好的文件名称组件
  k_str += ".mp4";
  if (k_eps)
    k_path.out_file /= k_eps->str();
  k_path.out_file /= k_str;

  if (!FSys::exists(k_path.out_file.parent_path()))
    FSys::create_directories(k_path.out_file.parent_path());

  /// 防止重复, 添加时间戳备份
  if (FSys::exists(k_path.out_file)) {
    FSys::backup_file(k_path.out_file);
  }

  p_video_path = k_path.out_file;
  p_image_sequence->set_path(k_path.image_list.front());
  p_image_sequence->create_video_asyn(k_path.out_file);

  FSys::open_explorer(k_path.out_file.parent_path());
  return k_term;
}
bool actn_image_to_movie::is_async() {
  return true;
}

actn_image_to_move_up::actn_image_to_move_up()
    : p_image_action(std::make_shared<actn_image_to_movie>()),
      p_up_path(std::make_shared<actn_up_paths>()) {
  p_name = "制作拍屏并上传";
  p_image_action->sig_get_arg.connect([this]() { return _arg_type; });

  /// 连接上传路径回调连接
  p_up_path->sig_get_arg.connect([this]() {
    auto k_vector = std::vector<FSys::path>{this->p_image_action->get_video_path()};
    return actn_up_paths::arg_{k_vector};
  });
}

bool actn_image_to_move_up::is_async() {
  return true;
}

long_term_ptr actn_image_to_move_up::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_term = this->get_long_term_signal();
  _arg_type = action_indirect::sig_get_arg().value();
  if (_arg_type.is_cancel) {
    this->cancel("取消上传");
    return k_term;
  }

  auto k_conv_image = p_image_action->run(in_data, in_parent);
  k_term->forward_sig(k_conv_image);
  k_term->forward_sig(p_up_path->get_long_term_signal());


  /// 运行转换完成后的上传文件回调
  k_conv_image->sig_finished.connect([this, in_parent]() {
    this->p_up_path->run({}, in_parent);
  });

  return k_term;
}
bool actn_image_to_move_up::is_accept(const arg_& in_any) {
  return p_image_action->is_accept(in_any);
}

}  // namespace doodle
