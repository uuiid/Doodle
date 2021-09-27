#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/libWarp/std_warp.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <Logger/Logger.h>
#include <Metadata/episodes.h>
#include <Metadata/shot.h>
#include <PinYin/convert.h>
#include <core/DoodleLib.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <opencv2/opencv.hpp>
namespace doodle {
std::string ImageSequence::clearString(const std::string &str) {
  auto &con  = convert::Get();
  auto str_r = std::string{};
  str_r      = con.toEn(str);

  return str_r;
}
ImageSequence::ImageSequence()
    : p_paths(),
      p_Text() {
}
ImageSequence::ImageSequence(const FSys::path &path_dir, const std::string &text)
    : std::enable_shared_from_this<ImageSequence>(),
      p_paths(),
      p_Text(std::move(clearString(text))) {
  set_path(path_dir);
}

bool ImageSequence::hasSequence() {
  return !p_paths.empty();
}

void ImageSequence::set_path(const FSys::path &dir) {
  this->seanDir(dir);
  for (auto &path : p_paths) {
    if (!FSys::is_regular_file(path)) {
      throw DoodleError("不是文件, 无法识别");
    }
  }
}

bool ImageSequence::seanDir(const FSys::path &dir) {
  if (!FSys::is_directory(dir))
    throw FileError{dir, "file not is a directory"};

  FSys::path ex{};
  for (auto &path : FSys::directory_iterator(dir)) {
    if (FSys::is_regular_file(path)) {
      if (ex.empty()) {
        ex = path.path().extension();
      }
      if (path.path().extension() == ex) {
        p_paths.emplace_back(path.path());
      }
    }
  }
  if (p_paths.empty())
    throw DoodleError("空目录");
  return true;
}

void ImageSequence::setText(const std::string &text) {
  p_Text = clearString(text);
}

std::string ImageSequence::set_shot_and_eps(const ShotPtr &in_shot, const EpisodesPtr &in_episodes) {
  auto k_str = CoreSet::getSet().getUser_en();  /// 基本水印, 名称
  /// 如果可以找到集数和镜头号直接添加上去, 否者就这样了
  if (in_shot && in_episodes) {
    k_str += fmt::format(" : {}_{}", in_episodes->str(), in_shot->str());
  } else if (in_shot) {
    k_str += fmt::format(" : {}", in_shot->str());
  } else if (in_episodes) {
    k_str += fmt::format(" : {}", in_episodes->str());
  }
  p_Text = k_str;

  /// 添加文件路径名称
  boost::replace_all(k_str, " ", "");   /// 替换不好的文件名称组件
  boost::replace_all(k_str, ":", "_");  /// 替换不好的文件名称组件
  k_str += ".mp4";

  if (in_episodes && !p_out_path.empty())
    p_out_path /= in_episodes->str();
  p_name = k_str;
  return p_Text;
}

void ImageSequence::create_video(const ImageSequence::asyn_arg_ptr &in_arg) {
  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  in_arg->long_sig->start();
  //检查父路径存在
  if (!FSys::exists(in_arg->out_path.parent_path()))
    FSys::create_directories(in_arg->out_path.parent_path());

  {
    const static cv::Size k_size{1920, 1080};
    auto video           = cv::VideoWriter{in_arg->out_path.generic_string(),
                                 cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                                 25,
                                 k_size};
    auto k_image         = cv::Mat{};
    auto k_image_resized = cv::Mat{};
    auto k_clone         = cv::Mat{};

    auto k_size_len      = in_arg->paths.size();

    //排序图片
    std::sort(in_arg->paths.begin(), in_arg->paths.end(),
              [](const FSys::path &k_r, const FSys::path &k_l) -> bool {
                return k_r.stem() < k_l.stem();
              });

    for (auto &&path : in_arg->paths) {
      k_image = cv::imread(path.generic_string());
      if (k_image.empty())
        throw DoodleError("open cv not read image");
      if (k_image.cols != k_size.width || k_image.rows != k_size.height)
        cv::resize(k_image, k_image_resized, k_size);
      else
        k_image_resized = k_image;

      {  //创建水印
        k_clone          = k_image_resized.clone();
        int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
        double fontScale = 1;
        int thickness    = 2;
        int baseline     = 0;
        auto textSize    = cv::getTextSize(in_arg->Text, fontFace,
                                           fontScale, thickness, &baseline);
        baseline += thickness;
        textSize.width += baseline;
        textSize.height += baseline;
        // center the text
        cv::Point textOrg((k_image_resized.cols - textSize.width) / 8,
                          (k_image_resized.rows + textSize.height) / 8);

        // draw the box
        cv::rectangle(k_clone, textOrg + cv::Point(0, baseline),
                      textOrg + cv::Point(textSize.width, -textSize.height),
                      cv::Scalar(0, 0, 0), -1);

        cv::addWeighted(k_clone, 0.7, k_image_resized, 0.3, 0, k_image_resized);
        // then put the text itself
        cv::putText(k_image_resized, in_arg->Text, textOrg, fontFace, fontScale,
                    cv::Scalar{0, 255, 255}, thickness, cv::LineTypes::LINE_AA);
      }

      in_arg->long_sig->sig_progress(rational_int{1, k_size_len});

      video << k_image_resized;
    }
  }
  in_arg->long_sig->sig_finished();
  in_arg->long_sig->sig_message_result(fmt::format("成功创建视频 {}\n", in_arg->out_path), long_term::warning);
}
void ImageSequence::set_path(const std::vector<FSys::path> &in_images) {
  p_paths = in_images;
}
void ImageSequence::set_out_dir(const FSys::path &out_dir) {
  p_out_path = out_dir;
}
void ImageSequence::create_video(const long_term_ptr &in_ptr) {
  if (!this->hasSequence())
    throw DoodleError{"not Sequence"};
  auto k_arg      = new_object<asyn_arg>();
  k_arg->out_path = p_out_path / p_name;
  k_arg->paths    = p_paths;
  k_arg->long_sig = in_ptr;
  k_arg->Text     = p_Text;
  ImageSequence::create_video(k_arg);
}
std::string ImageSequence::show_str(const std::vector<FSys::path> &in_images) {
  static std::regex reg{R"(\d+)"};
  std::smatch k_match{};
  std::vector<std::vector<std::double_t>> p_k_num;

  //  std::transform(in_images.begin(), in_images.end(), std::back_inserter(p_k_num),
  //                 [reg](const FSys::path &in_path) {
  //                   auto k_name = in_path.filename().generic_string();
  //                   auto k_b         = std::regex_iterator{k_name.begin(), k_name.end(), reg};
  //                   for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
  //
  //                   }
  //                 });
  //
  // boost::copy(
  //     in_images |
  //         boost::adaptors::transformed([](const FSys::path &in_path) -> std::vector<std::double_t> {
  //           auto k_name = in_path.filename().generic_string();
  //           auto rage =
  //               std::sregex_iterator{k_name.begin(), k_name.end(), reg} |
  //               boost::adaptors::transformed([]() -> std::double_t {});
  //         }),
  //     p_k_num);
  // for (const auto &image : in_images) {
  //   auto k_name = image.filename().generic_string();
  //   auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};
  //   std::vector<std::double_t> p_v{};
  //   for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
  //     p_v.emplace_back(std::stod(it->str()));
  //   }
  //   p_k_num.emplace_back(std::move(p_v));
  // }
  // const auto &k_size = p_k_num.front().size();
  // if (
  //     std::any_of(p_k_num.begin(), p_k_num.end(), [&k_size](const std::vector<std::double_t> &in) {
  //       return k_size == in.size();
  //     })) {
  //       for (auto i = 0; i < k_size; ++i)
  //       {
  //         std::

  //       }

  // }

  return std::string();
}
FSys::path ImageSequence::get_out_path() const {
  return p_out_path / p_name;
}

image_sequence_async::image_sequence_async()
    : p_image_sequence() {}
ImageSequencePtr image_sequence_async::set_path(const FSys::path &image_dir) {
  p_image_sequence = new_object<ImageSequence>();
  p_image_sequence->set_path(image_dir);
  return p_image_sequence;
}
ImageSequencePtr image_sequence_async::set_path(const std::vector<FSys::path> &image_path_list) {
  p_image_sequence = new_object<ImageSequence>();
  p_image_sequence->set_path(image_path_list);
  return p_image_sequence;
}
long_term_ptr image_sequence_async::create_video(const FSys::path &out_file) {
  auto k_term = new_object<long_term>();
  k_term->p_list.emplace_back(
      DoodleLib::Get().get_thread_pool()->enqueue(
          [self = p_image_sequence,out_file, k_term]() {
            self->set_path(out_file);
            self->create_video(k_term);
          }));
  return k_term;
}

}  // namespace doodle
