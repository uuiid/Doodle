//
// Created by TD on 2021/12/27.
//

#include "image_to_move.h"
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/thread_pool.h>

#include <opencv2/opencv.hpp>
#include <utility>

#include <opencv2/freetype.hpp>

namespace doodle {
namespace details {
namespace {
void watermark_add_image(cv::Mat &in_image, const image_watermark &in_watermark) {
  auto l_image     = in_image;
  int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
  double fontScale = 1;
  int thickness    = -1;
  int baseline     = 0;
  int fontHeight   = 60;
  int linestyle    = 8;
  cv::Ptr<cv::freetype::FreeType2> ft2{cv::freetype::createFreeType2()};
  ft2->loadFontData(std::string{doodle_config::font_default}, 0);
  auto textSize = ft2->getTextSize(in_watermark.p_text, fontHeight,
                                   thickness, &baseline);
  if (thickness > 0)
    baseline += thickness;
  textSize.width += baseline;
  textSize.height += baseline;
  // center the text
  cv::Point textOrg((in_image.cols - textSize.width) * in_watermark.p_width_proportion,
                    (in_image.rows + textSize.height) * in_watermark.p_height_proportion);

  // draw the box
  cv::rectangle(l_image, textOrg + cv::Point(0, baseline),
                textOrg + cv::Point(textSize.width, -textSize.height),
                cv::Scalar(0, 0, 0, 100), -1);

  cv::addWeighted(l_image, 0.7, in_image, 0.3, 0, in_image);
  // then put the text itself
  ft2->putText(in_image, in_watermark.p_text, textOrg,
               fontHeight, in_watermark.rgba,
               thickness, cv::LineTypes::LINE_AA, true);
}
}  // namespace

class image_to_move::impl {
 public:
  impl() = default;
  std::vector<image_file_attribute> p_image;
  FSys::path p_out_path;
  entt::handle p_h;
  std::future<void> result;

  std::atomic_bool stop{false};
};

// image_to_move::image_to_move(const entt::handle &in_handle, const std::vector<entt::handle> &in_vector)
//     : p_i(std::make_unique<impl>()) {
//   DOODLE_CHICK(in_handle.any_of<process_message>(),doodle_error{ "缺失进度指示结构"});
//   DOODLE_CHICK(in_handle.any_of<FSys::path>(),doodle_error{ "缺失输出文件路径"});
//   p_i->p_out_path = in_handle.get<FSys::path>();
//   std::for_each(in_vector.begin(), in_vector.end(), [](const entt::handle &in) {
//     DOODLE_CHICK(in.any_of<image_file_attribute>(),doodle_error{ "缺失文件属性"});
//   });
//   p_i->p_h = in_handle;
//   std::transform(in_vector.begin(), in_vector.end(), std::back_inserter(p_i->p_image),
//                  [](const entt::handle &in) -> image_file_attribute {
//                    return in.get<image_file_attribute>();
//                  });
//   std::for_each(p_i->p_image.begin(), p_i->p_image.end(), [](const image_file_attribute &in) {
//     DOODLE_CHICK(exists(in.file_path),doodle_error{ "找不到路径指向的文件"});
//   });
//   DOODLE_CHICK(!p_i->p_image.empty(),doodle_error{ "没有传入任何的图片"});
// }
image_to_move::image_to_move(const entt::handle &in_handle,
                             const std::vector<image_file_attribute> &in_vector)
    : p_i(std::make_unique<impl>()) {
  in_handle.any_of<process_message>() ? void() : throw_exception(doodle_error{"缺失进度指示结构"});
  in_handle.any_of<FSys::path>() ? void() : throw_exception(doodle_error{"缺失输出文件路径"});
  p_i->p_out_path = in_handle.get<FSys::path>();
  std::for_each(in_vector.begin(), in_vector.end(), [](const image_file_attribute &in) {
    exists(in.file_path) ? void() : throw_exception(doodle_error{"找不到路径指向的文件"});
  });
  p_i->p_image = in_vector;
  p_i->p_h     = in_handle;

  !p_i->p_image.empty() ? void() : throw_exception(doodle_error{"没有传入任何的图片"});
}

image_to_move::image_to_move(const entt::handle &in_handle,
                             const std::vector<FSys::path> &in_vector)
    : image_to_move(in_handle, make_default_attr(in_handle, in_vector)) {
}

std::vector<image_file_attribute> image_to_move::make_default_attr(
    const entt::handle &in_handle,
    const std::vector<FSys::path> &in_path_list) {
  std::vector<image_file_attribute> list{};
  list = in_path_list |
         ranges::views::transform(
             [&](const FSys::path &in_path) -> image_file_attribute {
               image_file_attribute l_attribute{};
               l_attribute.file_path = in_path;
               if (in_handle.any_of<shot>())
                 l_attribute.watermarks.emplace_back(fmt::to_string(in_handle.get<shot>()), 0.1, 0.1, rgb_default);
               if (in_handle.any_of<episodes>())
                 l_attribute.watermarks.emplace_back(fmt::to_string(in_handle.get<episodes>()), 0.1, 0.15, rgb_default);
               l_attribute.watermarks.emplace_back(g_reg()->ctx().at<user>().get_name(), 0.1, 0.2, rgb_default);
               l_attribute.watermarks.emplace_back(core_set::getSet().organization_name, 0.1, 0.25, rgb_default);
               return l_attribute;
             }) |
         ranges::to_vector;
  image_file_attribute::extract_num(list);
  const auto l_size = in_path_list.size();
  ranges::for_each(list, [&](image_file_attribute &in_attribute) {
    in_attribute.watermarks.emplace_back(fmt::format("{}/{}", in_attribute.num, l_size), 0.8, 0.1, rgb_default);
    in_attribute.watermarks.emplace_back(
        fmt::format("{:%Y-%m-%d %H:%M:%S}", chrono::floor<chrono::minutes>(chrono::system_clock::now())),
        0.8, 0.2, rgb_default);
  });

  return list;
}

image_to_move::~image_to_move() = default;

void image_to_move::init() {
  /// \brief 这里排序组件
  image_file_attribute::extract_num(p_i->p_image);
  std::sort(p_i->p_image.begin(), p_i->p_image.end());

  /// \brief 这里进行消息初始化
  auto &l_mag = p_i->p_h.patch<process_message>();
  l_mag.set_state(l_mag.run);
  l_mag.aborted_function = [self = this]() {
    if (self) {
      self->p_i->stop = true;
      self->abort();
    }
  };
  l_mag.message(fmt::format("获得图片路径 {}", p_i->p_image.front().file_path.parent_path()));

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!p_i->p_out_path.has_extension() && p_i->p_h.any_of<episodes, shot>())
    p_i->p_out_path /= fmt::format(
        "{}_{}.mp4",
        p_i->p_h.any_of<episodes>() ? fmt::to_string(p_i->p_h.get<episodes>()) : "eps_none"s,
        p_i->p_h.any_of<shot>() ? fmt::to_string(p_i->p_h.get<shot>()) : "sh_none"s);
  else if (!p_i->p_out_path.has_extension()) {
    p_i->p_out_path /= fmt::format(
        "{}.mp4", core_set::getSet().get_uuid());
  } else
    p_i->p_out_path.extension() == ".mp4" ? void() : throw_exception(doodle_error{"扩展名称不是MP4"});

  if (exists(p_i->p_out_path.parent_path()))
    create_directories(p_i->p_out_path.parent_path());

  l_mag.message(fmt::format("开始创建视频 {}", p_i->p_out_path));
  l_mag.set_name(p_i->p_out_path.filename().generic_string());

  auto k_fun = [&]() -> void {
    const static cv::Size k_size{1920, 1080};
    auto video             = cv::VideoWriter{p_i->p_out_path.generic_string(),
                                 cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                                 25,
                                 k_size};
    auto k_image           = cv::Mat{};
    const auto &k_size_len = p_i->p_image.size();
    for (auto &l_image : p_i->p_image) {
      if (p_i->stop)
        return;
      p_i->p_h.patch<process_message>([&](process_message &in_message) {
        in_message.message(fmt::format("开始读取图片 {}", l_image.file_path));
      });
      k_image = cv::imread(l_image.file_path.generic_string());
      if (k_image.empty()) {
        DOODLE_LOG_ERROR("{} 图片读取失败 跳过", l_image.file_path);
        continue;
      }
      if (k_image.cols != k_size.width || k_image.rows != k_size.height)
        cv::resize(k_image, k_image, k_size);

      for (auto &k_w : l_image.watermarks) {
        watermark_add_image(k_image, k_w);
      }
      p_i->p_h.patch<process_message>([&](process_message &in_message) {
        in_message.progress_step(rational_int{1, k_size_len});
      });
      video << k_image;
    }
  };
  p_i->result = std::move(g_thread_pool().enqueue(k_fun));
}
void image_to_move::update(
    const chrono::duration<chrono::system_clock::rep,
                           chrono::system_clock::period> &,
    void *data) {
  p_i->result.valid() ? void() : throw_exception(doodle_error{"无效的数据"});
  switch (p_i->result.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->result.get();
        this->succeed();
      } catch (const doodle_error &error) {
        DOODLE_LOG_ERROR(boost::diagnostic_information(error.what()));
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}
void image_to_move::succeeded() {
  p_i->p_h.patch<process_message>([&](process_message &in) {
    in.set_state(in.success);
    auto k_str = fmt::format("成功完成任务\n");
    in.message(k_str, in.warning);
  });
  p_i->p_h.emplace_or_replace<FSys::path>(p_i->p_out_path);
}
void image_to_move::failed() {
  p_i->p_h.patch<process_message>([&](process_message &in) {
    in.set_state(in.fail);
    auto k_str = fmt::format("转换失败 \n");
    in.message(k_str, in.warning);
  });
}
void image_to_move::aborted() {
  p_i->stop = true;
  p_i->p_h.patch<process_message>([&](process_message &in) {
    in.set_state(in.fail);
    auto k_str = fmt::format("合成视频被主动结束 合成视频文件将被主动删除\n");
    in.message(k_str, in.warning);
  });
  try {
    remove(p_i->p_out_path);
  } catch (const FSys::filesystem_error &err) {
    p_i->p_h.patch<process_message>([&](process_message &in) {
      auto k_str = fmt::format("合成视频主动删除失败 {}\n", boost::diagnostic_information(err.what()));
      in.message(k_str, in.warning);
    });
    throw;
  }
}

}  // namespace details
image_watermark::image_watermark(std::string in_p_text,
                                 double_t in_p_width_proportion,
                                 double_t in_p_height_proportion,
                                 cv::Scalar in_rgba)
    : p_text(std::move(in_p_text)),
      p_width_proportion(in_p_width_proportion),
      p_height_proportion(in_p_height_proportion),
      rgba(std::move(in_rgba)) {}
bool image_file_attribute::operator<(const image_file_attribute &in_rhs) const {
  return num < in_rhs.num;
}
bool image_file_attribute::operator>(const image_file_attribute &in_rhs) const {
  return in_rhs < *this;
}
bool image_file_attribute::operator<=(const image_file_attribute &in_rhs) const {
  return !(in_rhs < *this);
}
bool image_file_attribute::operator>=(const image_file_attribute &in_rhs) const {
  return !(*this < in_rhs);
}
void image_file_attribute::extract_num_list() {
  static std::regex reg{R"(\d+)"};
  std::smatch k_match{};

  auto k_name = file_path.filename().generic_string();

  auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};

  for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
    k_match = *it;
    num_list.push_back(std::stoi(k_match.str()));
  }
}
void image_file_attribute::extract_num(std::vector<image_file_attribute> &in_image_list) {
  for (auto &in : in_image_list)
    in.extract_num_list();

  const auto k_size = in_image_list.front().num_list.size();

  std::all_of(in_image_list.begin(), in_image_list.end(),
              [k_size](const image_file_attribute &in) -> bool {
                return in.num_list.size() == k_size;
              })
      ? void()
      : throw_exception(doodle_error{"序列不匹配 {}"s, in_image_list.front().file_path});

  in_image_list.size() >= 2 ? void() : throw_exception(doodle_error{"单个文件, 无法搜索帧号"});
  auto &one   = in_image_list[0].num_list;
  auto &tow   = in_image_list[1].num_list;
  auto l_item = ranges::views::ints(std::size_t{0}, k_size) |
                ranges::views::filter([&](const std::size_t &in_tuple) {
                  return one[in_tuple] != tow[in_tuple];
                }) |
                ranges::to_vector;

  !l_item.empty() ? void() : throw_exception(doodle_error{"没有找到帧索引"});
  auto l_index = l_item.front();
  std::for_each(in_image_list.begin(), in_image_list.end(),
                [&](image_file_attribute &in_attribute) {
                  in_attribute.num = in_attribute.num_list[l_index];
                });
}
image_file_attribute::image_file_attribute() : num(){};
image_file_attribute::image_file_attribute(FSys::path in_path)
    : image_file_attribute() {
  file_path = std::move(in_path);
}
}  // namespace doodle
