#include <Logger/logger.h>
#include <Metadata/episodes.h>
#include <Metadata/shot.h>
#include <core/doodle_lib.h>
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/metadata/assets_path.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <pin_yin/convert.h>

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

namespace details {
const std::vector<std::int32_t> &image_file::extract_num() {
  static std::regex reg{R"(\d+)"};
  std::smatch k_match{};

  auto k_name = p_file.filename().generic_string();

  auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};

  for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
    k_match = *it;
    p_list.push_back(std::stoi(k_match.str()));
  }
  return p_list;
}
void image_file::set_path(const FSys::path &in_) {
  p_file = in_;
  if (FSys::exists(p_file) && p_file.has_filename())
    extract_num();
}
std::int32_t image_file::get_frame() const {
  return p_frame;
}
bool image_file::speculate_frame(const image_file &in) {
  if (p_list.size() == in.p_list.size()) {
    for (auto i = 0; i < p_list.size(); ++i) {
      if (p_list[i] != in.p_list[i]) {
        p_frame = p_list[i];
        p_index = i;
        return true;
      }
    }
  }
  return false;
}
bool image_file::next(image_file &in) const {
  if (p_list.size() == in.p_list.size()) {
    in.p_frame = in.p_list[p_index];
    in.p_index = p_index;
    return true;
  }
  return false;
}
bool image_file::operator<(const image_file &in_rhs) const {
  return p_frame < in_rhs.p_frame;
}
bool image_file::operator>(const image_file &in_rhs) const {
  return in_rhs < *this;
}
bool image_file::operator<=(const image_file &in_rhs) const {
  return !(in_rhs < *this);
}
bool image_file::operator>=(const image_file &in_rhs) const {
  return !(*this < in_rhs);
}
image_file::operator bool() const {
  return p_index >= 0;
}
bool image_file::operator==(const image_file &in_rhs) const {
  return p_file == in_rhs.p_file;
}
bool image_file::operator!=(const image_file &in_rhs) const {
  return !(in_rhs == *this);
}

class watermark::impl {
 public:
  string p_text;
  std::function<string(std::int32_t in_frame)> p_text_fun;

  std::double_t p_width_proportion;
  std::double_t p_height_proportion;

  cv::Scalar color{};

  void add_to_image(cv::Mat &in_image, std::int32_t in_frame);
};

void watermark::impl::add_to_image(cv::Mat &in_image, std::int32_t in_frame) {
  auto k_text      = p_text_fun ? p_text_fun(in_frame) : p_text;

  auto l_image     = in_image;
  int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
  double fontScale = 1;
  int thickness    = 2;
  int baseline     = 0;
  auto textSize    = cv::getTextSize(k_text, fontFace,
                                     fontScale, thickness, &baseline);
  baseline += thickness;
  textSize.width += baseline;
  textSize.height += baseline;
  // center the text
  cv::Point textOrg((in_image.cols - textSize.width) * this->p_width_proportion,
                    (in_image.rows + textSize.height) * this->p_height_proportion);

  // draw the box
  cv::rectangle(l_image, textOrg + cv::Point(0, baseline),
                textOrg + cv::Point(textSize.width, -textSize.height),
                cv::Scalar(0, 0, 0), -1);

  cv::addWeighted(l_image, 0.7, in_image, 0.3, 0, in_image);
  // then put the text itself
  cv::putText(in_image, k_text, textOrg, fontFace, fontScale,
              this->color, thickness, cv::LineTypes::LINE_AA);
}

watermark::watermark()
    : p_impl(std::make_unique<impl>()) {
  set_text_point(0.125, 0.125);
  set_text_color(0, 255, 255);
}
void watermark::path_to_ep_sc(const FSys::path &in_path) {
  shot k_s{};
  episodes k_ep{};
  const auto k_b_s = k_s.analysis(in_path);
  const auto k_b_e = k_ep.analysis(in_path);

  auto k_str       = core_set::getSet().get_user_en();  /// 基本水印, 名称
  /// 如果可以找到集数和镜头号直接添加上去, 否者就这样了
  if (k_b_s && k_b_e) {
    k_str += fmt::format(" : {}_{}", k_ep.str(), k_s.str());
  } else if (k_b_s) {
    k_str += fmt::format(" : {}", k_s.str());
  } else if (k_b_e) {
    k_str += fmt::format(" : {}", k_ep.str());
  }
  p_impl->p_text = k_str;
}
string watermark::get_clear_str() const {
  auto k_str = p_impl->p_text;
  boost::replace_all(k_str, " ", "");   /// 替换不好的文件名称组件
  boost::replace_all(k_str, ":", "_");  /// 替换不好的文件名称组件
  return convert::Get().toEn(k_str);
}

watermark::~watermark() = default;

void watermark::set_text(const string &in_string) {
  p_impl->p_text = in_string;
}

void watermark::set_text_point(std::double_t in_width_proportion,
                               std::double_t in_height_proportion) {
  p_impl->p_height_proportion = in_height_proportion;
  p_impl->p_width_proportion  = in_width_proportion;
}
void watermark::set_text_color(std::double_t v0,
                               std::double_t v1,
                               std::double_t v2,
                               std::double_t v3) {
  p_impl->color = {v0, v1, v2, v3};
}
void watermark::set_text(std::function<string(std::int32_t in_frame)> &&in_) {
  p_impl->p_text_fun = std::move(in_);
}

}  // namespace details

image_sequence::image_sequence()
    : p_paths(),
      p_pow() {
}
image_sequence::image_sequence(const FSys::path &path_dir)
    : std::enable_shared_from_this<image_sequence>(),
      p_paths() {
  set_path(path_dir);
}

bool image_sequence::has_sequence() {
  return !p_paths.empty();
}

void image_sequence::set_path(const FSys::path &dir) {
  this->seanDir(dir);
  for (auto &path : p_paths) {
    if (!FSys::is_regular_file(path)) {
      throw doodle_error("不是文件, 无法识别");
    }
  }
}

bool image_sequence::seanDir(const FSys::path &dir) {
  if (!FSys::is_directory(dir))
    throw file_error{dir, "file not is a directory"};

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
    throw doodle_error("空目录");
  return true;
}
void image_sequence::set_gamma(std::double_t in_pow) {
  p_pow = in_pow;
}
void image_sequence::set_path(const std::vector<FSys::path> &in_images) {
  p_paths = in_images;
}
void image_sequence::set_out_path(const FSys::path &out_dir) {
  p_out_path = out_dir;
}
void image_sequence::create_video(const long_term_ptr &in_ptr) {
  if (!this->has_sequence())
    throw doodle_error{"not Sequence"};
  in_ptr->start();
  //检查父路径存在
  if (!FSys::exists(p_out_path.parent_path()))
    FSys::create_directories(p_out_path.parent_path());

  {
    const static cv::Size k_size{1920, 1080};
    auto video      = cv::VideoWriter{p_out_path.generic_string(),
                                 cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                                 25,
                                 k_size};
    auto k_image    = cv::Mat{};
    auto k_size_len = p_paths.size();

    //排序图片
    std::sort(p_paths.begin(), p_paths.end(),
              [](const FSys::path &k_r, const FSys::path &k_l) -> bool {
                return k_r.stem() < k_l.stem();
              });

    for (std::int32_t i = 0; i < p_paths.size(); ++i) {
      auto path = p_paths[i];
      k_image   = cv::imread(path.generic_string());
      if (k_image.empty())
        throw doodle_error("open cv not read image");
      if (k_image.cols != k_size.width || k_image.rows != k_size.height)
        cv::resize(k_image, k_image, k_size);

      for (auto &k_w : p_watermark_list) {
        k_w.p_impl->add_to_image(k_image, i);
      }
      in_ptr->sig_progress(rational_int{1, k_size_len});

      if (p_pow) {
        cv::Mat k_d{};
        k_image.convertTo(k_d,CV_64F);
        cv::pow(k_d, *p_pow, k_d);
        k_d.convertTo(k_image,CV_8U);
      }

      video << k_image;
    }
  }
  in_ptr->sig_finished();
  in_ptr->sig_message_result(fmt::format("成功创建视频 {}\n", p_out_path), long_term::warning);
}
bool image_sequence::is_image_sequence(const std::vector<FSys::path> &in_file_list) {
  std::vector<details::image_file_ptr> k_files{};
  boost::copy(
      in_file_list |
          boost::adaptors::transformed(
              [](const FSys::path &in_path) -> details::image_file_ptr {
                return new_object<details::image_file>(in_path);
              }),
      std::back_inserter(k_files));
  if (k_files.empty())
    return false;
  auto k_f = k_files.back();
  k_files.pop_back();
  for (auto &i : k_files) {
    if (k_f->speculate_frame(*i))
      break;
  }
  if (!(*k_f))
    return false;

  for (auto &i : k_files) {
    if (!k_f->next(*i))
      return false;
  }
  k_files.push_back(k_f);
  boost::sort(k_files, &boost::less_pointees<details::image_file_ptr>);

  auto k_i = k_files.front()->get_frame();
  for (auto &i : k_files) {
    if (k_i == i->get_frame())
      ++k_i;
    else
      return false;
  }

  return true;
}
std::string image_sequence::show_str(const std::vector<FSys::path> &in_images) {
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
FSys::path image_sequence::get_out_path() const {
  return p_out_path / p_name;
}
void image_sequence::add_watermark(const details::watermark &in_watermark) {
  p_watermark_list.push_back(std::move(in_watermark));
}

image_sequence_async::image_sequence_async()
    : p_image_sequence() {}
image_sequence_ptr image_sequence_async::set_path(const FSys::path &image_dir) {
  p_image_sequence = new_object<image_sequence>();
  p_image_sequence->set_path(image_dir);
  return p_image_sequence;
}
image_sequence_ptr image_sequence_async::set_path(const std::vector<FSys::path> &image_path_list) {
  p_image_sequence = new_object<image_sequence>();
  p_image_sequence->set_path(image_path_list);
  return p_image_sequence;
}
// image_sequence_ptr image_sequence_async::set_path(const entt::handle &in_path) {
//   set_path(in_path.get<assets_path_vector>().get_local_path());
//   auto k_out_dir = in_path.get<assets_path_vector>().get().front().get_cache_path();
//   p_image_sequence->set_out_path(k_out_dir);

//   return p_image_sequence;
// }
long_term_ptr image_sequence_async::create_video(const FSys::path &out_file) {
  p_image_sequence->set_path(out_file);
  return create_video();
}
long_term_ptr image_sequence_async::create_video() {
  auto k_term = new_object<long_term>();
  k_term->set_name(fmt::format("合成视频 {}", p_image_sequence->get_out_path().filename()));
  k_term->p_list.emplace_back(
      doodle_lib::Get().get_thread_pool()->enqueue(
          [self = p_image_sequence, k_term]() {
            self->create_video(k_term);
          }));
  return k_term;
}

}  // namespace doodle
