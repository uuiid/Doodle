#include <doodle_core/metadata/image_size.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <opencv2/opencv.hpp>

namespace doodle::http {

struct video_info_t {
  std::double_t duration_{0.0};
  cv::Size size_{0, 0};
  // 帧率
  std::int32_t fps_{0};
};

video_info_t get_video_duration(const FSys::path& in_path);
std::tuple<cv::Size, double, FSys::path> handle_video_file(
    const FSys::path& in_path, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file, const progress_data_ptr& in_progress_data = nullptr
);

struct image_info_t {
  FSys::path path_;
  cv::Size size_{0, 0};
};
image_info_t convert_to_png(const FSys::path& in_path);

/// 按 preview_file 的 uuid 生成全套图片变体（缩略图 150x100 / 方形 100x100 /
/// 预览 1200x / 外包加水印 1200x），路径取自 kitsu_ctx_t。
/// 返回原图尺寸（调用方用它填 preview_file.width_ / height_）。
///
/// 为什么单独导出：AI 灯光定稿候选要注册成 preview_file，必须生成与既有预览文件
/// **完全一致**的一套变体，否则它在预览列表页里会缺图或尺寸不对。
/// 尺寸表与加水印逻辑都在这一个函数里，复制一份必然会漂移。
cv::Size save_variants(const cv::Mat& in_image, const uuid& in_id);
}  // namespace doodle::http