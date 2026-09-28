//
// AI 灯光：参考图统计层测试（docs/ue_ai_lighting_plan.md §5.1 的 ①）
//
// 这份盯的是 ai_light_stats 的两个函数，**只喂合成图**：
// 不碰数据库、不碰 HTTP、不依赖任何真实素材 —— 统计的正确性必须能脱开素材复现。
// 唯一的例外是最后一个用例，它要验「读已落盘的文件」这条路径，所以会写一个临时 PNG。
//
// 为什么用合成图而不是「拿一张参考图和期望值对」：
// 期望值必须能**手算**出来，否则测试只是把当前实现的行为固化下来。
// 例如中灰 128 的三通道相等，线性化后 x/y 必然落在 sRGB 白点上 (0.3127, 0.3290)，
// 代入 McCamy 恰好是 6505K —— 这是一个能独立验证整条链的锚点，
// 公式符号写反（分母该是 0.1858 − y）会立刻在这里炸。
//

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/http_method/kitsu/ai_light_stats.h>

#include <boost/test/unit_test.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>

namespace doodle::ai_light {
namespace {

/// 纯色 BGR 图（cv::Scalar 按 BGR 序，与 imread 一致）。
cv::Mat solid_bgr(int in_rows, int in_cols, const cv::Scalar& in_bgr) {
  return cv::Mat(in_rows, in_cols, CV_8UC3, in_bgr);
}

/// 水平 0..255 渐变（三通道同值）：列 c 的值 = 255·c/(cols−1)。
/// 宽度 256 时列值恰好等于 0..255，于是分位可以手算。
cv::Mat horizontal_ramp(int in_rows, int in_cols) {
  cv::Mat l_img(in_rows, in_cols, CV_8UC3);
  for (int y = 0; y < in_rows; ++y)
    for (int x = 0; x < in_cols; ++x) {
      const auto l_v = static_cast<uchar>(std::lround(255.0 * x / (in_cols - 1)));
      l_img.at<cv::Vec3b>(y, x) = cv::Vec3b(l_v, l_v, l_v);
    }
  return l_img;
}

/// 切比雪夫径向衰减：中心最亮，四条边的中点最暗（dist = 1）。
/// in_strength = 0.9 时边缘只剩 10% 亮度，足以让暗角签名满分。
cv::Mat radial_falloff(int in_rows, int in_cols, double in_strength) {
  cv::Mat l_img(in_rows, in_cols, CV_8UC3);
  const double l_cx = (in_cols - 1) / 2.0;
  const double l_cy = (in_rows - 1) / 2.0;
  for (int y = 0; y < in_rows; ++y)
    for (int x = 0; x < in_cols; ++x) {
      const double l_d = std::max(std::abs(x - l_cx) / l_cx, std::abs(y - l_cy) / l_cy);
      const auto l_v   = static_cast<uchar>(std::lround(255.0 * (1.0 - in_strength * l_d)));
      l_img.at<cv::Vec3b>(y, x) = cv::Vec3b(l_v, l_v, l_v);
    }
  return l_img;
}

/// 显式走 ADL 的 to_json。**不要**写 nlohmann::json{stats} ——
/// 花括号是列表初始化，会静默变成单元素数组（见 ai_light/common.h 顶部）。
nlohmann::json stats_to_json(const reference_image_stats_t& in_stats) {
  nlohmann::json l_json;
  to_json(l_json, in_stats);
  return l_json;
}

/// 两图的统计是否逐位相同
bool same_stats(const reference_analysis_t& in_a, const reference_analysis_t& in_b) {
  return stats_to_json(in_a.stats_) == stats_to_json(in_b.stats_);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_stats)

// ---------------------------------------------------------------------------
// 亮度分位与对比度
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(black_image_is_all_zero_and_hits_confidence_floor) {
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{0, 0, 0}));

  // 统计在 8bit 等级上做，所以等级 0 恰好得 0（不是「一个 bin 之内的近似值」）
  BOOST_CHECK_SMALL(l_res.stats_.luma_percentiles_.p10_, 1e-12);
  BOOST_CHECK_EQUAL(l_res.stats_.luma_percentiles_.p10_, l_res.stats_.luma_percentiles_.p90_);
  BOOST_CHECK_SMALL(l_res.stats_.contrast_, 1e-12);
  // 全黑 = 每个像素都被暗部削波
  BOOST_CHECK_CLOSE(l_res.stats_.clip_rate_.low_, 1.0, 1e-9);
  BOOST_CHECK_SMALL(l_res.stats_.clip_rate_.high_, 1e-12);
  // 中心本来就黑 → 暗角比值无意义 → 痕迹 0
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
  // 对比度 0 → content_factor 0.1，再乘分辨率因子，落到 0.05 地板
  BOOST_CHECK_CLOSE(l_res.confidence_, 0.05, 1e-9);
}

BOOST_AUTO_TEST_CASE(white_image_is_fully_clipped_high) {
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{255, 255, 255}));

  BOOST_CHECK_CLOSE(l_res.stats_.luma_percentiles_.p50_, 1.0, 1e-9);
  BOOST_CHECK_CLOSE(l_res.stats_.clip_rate_.high_, 1.0, 1e-9);
  BOOST_CHECK_SMALL(l_res.stats_.clip_rate_.low_, 1e-12);
  BOOST_CHECK_SMALL(l_res.stats_.contrast_, 1e-12);
  BOOST_CHECK_CLOSE(l_res.confidence_, 0.05, 1e-9);
  // 四边与中心一样亮 → 不是暗角
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
}

BOOST_AUTO_TEST_CASE(horizontal_ramp_percentiles_and_contrast) {
  // 256 列、列值 0..255、每列 64 像素 → 每个 bin 恰好 64 个样本，分位可以手算
  const auto l_res = analyze_display_image(horizontal_ramp(64, 256));

  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p10_ - 0.10), 0.01);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p50_ - 0.50), 0.01);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p90_ - 0.90), 0.01);
  // 对比度是 p90 − p10（不是标准差：见 ai_light_stats.cpp 里的说明）
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.contrast_ - 0.80), 0.01);
  // 最左一列全 0、最右一列全 255 → 两端各占 1/256
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.clip_rate_.low_ - 1.0 / 256.0), 1e-9);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.clip_rate_.high_ - 1.0 / 256.0), 1e-9);
  // 右边亮、左边暗：四条边不是一致变暗，所以不是暗角
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
  // 对比度 0.8 不触发「近乎纯色」分支，所以置信度高于地板
  BOOST_CHECK_GT(l_res.confidence_, 0.06);
}

// ---------------------------------------------------------------------------
// 色温：整条「sRGB → 线性光 → XYZ → xy → McCamy」链的锚点
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(neutral_gray_reads_as_d65_white_point) {
  // 中灰三通道相等 → 线性化后仍相等 → x/y 必然落在 sRGB 白点 (0.3127, 0.3290)，
  // 代入 McCamy 得 ~6505K。这个值可以独立手算，不是「看实现输出什么就断言什么」。
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{128, 128, 128}));

  BOOST_CHECK_SMALL(std::abs(l_res.stats_.white_balance_kelvin_ - 6500.0), 200.0);
  BOOST_CHECK_CLOSE(l_res.stats_.luma_percentiles_.p50_, 128.0 / 255.0, 1.0);
}

BOOST_AUTO_TEST_CASE(warm_and_cool_solids_are_ordered_correctly) {
  // RGB(200,150,110) → BGR(110,150,200)
  const auto l_warm = analyze_display_image(solid_bgr(100, 100, cv::Scalar{110, 150, 200}));
  BOOST_CHECK_GT(l_warm.stats_.white_balance_kelvin_, 2500.0);
  BOOST_CHECK_LT(l_warm.stats_.white_balance_kelvin_, 4200.0);

  // 冷暖对调 → 明显更冷
  const auto l_cool = analyze_display_image(solid_bgr(100, 100, cv::Scalar{200, 150, 110}));
  BOOST_CHECK_GT(l_cool.stats_.white_balance_kelvin_, l_warm.stats_.white_balance_kelvin_ + 1000.0);
}

BOOST_AUTO_TEST_CASE(saturated_primary_falls_back_instead_of_reporting_a_bogus_kelvin) {
  // 纯红永远不在普朗克轨迹附近（x ≈ 0.64）。不设 sanity 盒的话 McCamy 会给出 ~2650K ——
  // 一个看着精确、其实毫无意义的数字。要求回落到 D65 中性。
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{0, 0, 255}));

  BOOST_CHECK_GT(l_res.stats_.white_balance_kelvin_, 6000.0);
}

// ---------------------------------------------------------------------------
// 调色板
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(palette_has_k_entries_sorted_by_share_and_is_rgb_ordered) {
  // 5 条等宽竖条，5 个互相远离的颜色 —— 与 g_palette_k 相等，聚簇结果确定
  cv::Mat l_img(100, 200, CV_8UC3);
  const cv::Vec3b l_colors[] = {
      cv::Vec3b{0, 0, 255},      // 红
      cv::Vec3b{0, 255, 0},      // 绿
      cv::Vec3b{255, 0, 0},      // 蓝
      cv::Vec3b{0, 255, 255},    // 黄
      cv::Vec3b{255, 255, 0},    // 青
  };
  for (int y = 0; y < l_img.rows; ++y)
    for (int x = 0; x < l_img.cols; ++x) l_img.at<cv::Vec3b>(y, x) = l_colors[x / 40];

  const auto l_res = analyze_display_image(l_img);

  BOOST_REQUIRE_EQUAL(l_res.stats_.palette_.size(), 5u);
  // 占比降序、和为 1（丢簇后重新归一化过）
  double l_sum = 0.0;
  for (std::size_t i = 0; i < l_res.stats_.palette_.size(); ++i) {
    l_sum += l_res.stats_.palette_[i].share_;
    BOOST_CHECK_CLOSE(l_res.stats_.palette_[i].share_, 0.2, 1.0);
    if (i > 0) BOOST_CHECK_GE(l_res.stats_.palette_[i - 1].share_, l_res.stats_.palette_[i].share_);
  }
  BOOST_CHECK_CLOSE(l_sum, 1.0, 1e-6);

  // **通道序**：契约里的 vec3_t 是 RGB。若忘了把 k-means 中心从 BGR 换序，
  // 红色条会以 (0,0,1) 出现 —— 这两条断言就是拿来钉死这一点的。
  const auto l_is_red = [](const image_palette_entry_t& in_e) {
    return in_e.color_.x_ > 0.9 && in_e.color_.y_ < 0.1 && in_e.color_.z_ < 0.1;
  };
  const auto l_is_blue = [](const image_palette_entry_t& in_e) {
    return in_e.color_.z_ > 0.9 && in_e.color_.x_ < 0.1 && in_e.color_.y_ < 0.1;
  };
  BOOST_CHECK(std::any_of(l_res.stats_.palette_.begin(), l_res.stats_.palette_.end(), l_is_red));
  BOOST_CHECK(std::any_of(l_res.stats_.palette_.begin(), l_res.stats_.palette_.end(), l_is_blue));
}

BOOST_AUTO_TEST_CASE(analysis_is_deterministic) {
  // k-means++ 不设随机种子时每次初始化不同；这条用例钉死 cv::setRNGSeed 必须留着。
  const auto l_image = radial_falloff(64, 96, 0.5);
  const auto l_first = analyze_display_image(l_image);
  const auto l_again = analyze_display_image(l_image);

  BOOST_CHECK(same_stats(l_first, l_again));
  BOOST_CHECK_EQUAL(l_first.confidence_, l_again.confidence_);
}

// ---------------------------------------------------------------------------
// 输入形态的宽容与拒绝
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(gray_and_bgr_inputs_agree) {
  cv::Mat l_ramp_gray = horizontal_ramp(32, 128);
  cv::cvtColor(l_ramp_gray, l_ramp_gray, cv::COLOR_BGR2GRAY);

  cv::Mat l_ramp_bgr;
  cv::cvtColor(l_ramp_gray, l_ramp_bgr, cv::COLOR_GRAY2BGR);

  const auto l_from_gray = analyze_display_image(l_ramp_gray);
  const auto l_from_bgr  = analyze_display_image(l_ramp_bgr);
  BOOST_CHECK(same_stats(l_from_gray, l_from_bgr));
}

BOOST_AUTO_TEST_CASE(bgra_input_drops_alpha) {
  const auto l_bgr = horizontal_ramp(32, 128);
  cv::Mat l_bgra;
  cv::cvtColor(l_bgr, l_bgra, cv::COLOR_BGR2BGRA);

  BOOST_CHECK(same_stats(analyze_display_image(l_bgra), analyze_display_image(l_bgr)));
}

BOOST_AUTO_TEST_CASE(empty_and_non_8bit_inputs_are_rejected) {
  BOOST_CHECK_THROW(analyze_display_image(cv::Mat{}), doodle_error);

  cv::Mat l_f32;
  solid_bgr(16, 16, cv::Scalar{10, 20, 30}).convertTo(l_f32, CV_32F);
  // 8bit 与 32F 的数值域差 255 倍：这里必须抛，而不是「顺手归一化一下」
  BOOST_CHECK_THROW(analyze_display_image(l_f32), doodle_error);
}

// ---------------------------------------------------------------------------
// 后处理痕迹（暗角签名）
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(symmetric_vignette_is_detected_and_asymmetric_darkening_is_not) {
  // 径向衰减：四条边的中点都远暗于中心 → 暗角
  const auto l_vignette = analyze_display_image(radial_falloff(64, 256, 0.9));
  BOOST_CHECK_GT(l_vignette.stats_.retouch_score_, 0.5);

  // 只有左边一条竖带被压暗：四条边不是一致变暗，属于**内容**而不是暗角。
  // 这条用例是「用边中点而不是整条外缘带」的原因 —— 整条带在角上互相重叠，
  // 单侧压暗会把相邻两条边一起拉低，于是任何单侧压暗都会被误判成暗角。
  cv::Mat l_one_sided = solid_bgr(200, 200, cv::Scalar{128, 128, 128});
  l_one_sided(cv::Rect(0, 0, 20, 200)).setTo(cv::Scalar{40, 40, 40});
  const auto l_asymmetric = analyze_display_image(l_one_sided);
  BOOST_CHECK_SMALL(l_asymmetric.stats_.retouch_score_, 1e-12);
}

BOOST_AUTO_TEST_CASE(retouch_score_lowers_confidence) {
  // 两张同分辨率（256×64）的图：渐变（痕迹 0）与径向衰减（痕迹满分）。
  // 分辨率因子与 content_factor 都相同，所以置信度的差别只可能来自痕迹那一项。
  const auto l_clean    = analyze_display_image(horizontal_ramp(64, 256));
  const auto l_retouch  = analyze_display_image(radial_falloff(64, 256, 0.9));

  BOOST_CHECK_SMALL(l_clean.stats_.retouch_score_, 1e-12);
  BOOST_CHECK_GT(l_retouch.stats_.retouch_score_, 0.5);
  // 权重是 0.5，所以满痕迹正好打对折
  BOOST_CHECK_LT(l_retouch.confidence_, l_clean.confidence_ * 0.6);
  // 但不会打到地板：参考图本来就该是修过的，痕迹不该让统计「归零」
  BOOST_CHECK_GT(l_retouch.confidence_, 0.05);
}

// ---------------------------------------------------------------------------
// 置信度的分辨率因子
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(confidence_grows_with_resolution) {
  const auto l_small = analyze_display_image(horizontal_ramp(180, 320));
  const auto l_big   = analyze_display_image(horizontal_ramp(720, 1280));

  BOOST_CHECK_GT(l_big.confidence_, l_small.confidence_);
  // 1280×720 正好是基准 → 因子 1.0，配合 content_factor 1.0、痕迹 0 → 满置信度
  BOOST_CHECK_CLOSE(l_big.confidence_, 1.0, 1e-6);
}

// ---------------------------------------------------------------------------
// 读已落盘的文件
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(analyze_reference_file_reads_png_and_rejects_junk) {
  const auto l_dir = FSys::temp_directory_path();
  const auto l_png = l_dir / "doodle_test_ai_light_stats_ramp.png";
  const auto l_fake = l_dir / "doodle_test_ai_light_stats_fake.png";
  const auto l_missing = l_dir / "doodle_test_ai_light_stats_missing.png";

  const auto l_image = horizontal_ramp(64, 256);
  BOOST_REQUIRE(cv::imwrite(l_png.generic_string(), l_image));

  const auto l_from_file = analyze_reference_file(l_png);
  BOOST_REQUIRE(l_from_file.has_value());
  // PNG 无损往返，所以「读文件」与「直接喂 Mat」必须给出逐位相同的结果
  BOOST_CHECK(same_stats(*l_from_file, analyze_display_image(l_image)));

  BOOST_CHECK(!analyze_reference_file(l_missing).has_value());

  // 扩展名是 .png 但内容是文本：参考图上传路径必须能识别出这种文件，
  // 否则库里会留下一条尺寸 0×0、看起来只是「统计不可用」的坏记录。
  {
    std::ofstream l_out(l_fake, std::ios::binary);
    l_out << "definitely not a png";
  }
  BOOST_CHECK(!analyze_reference_file(l_fake).has_value());

  FSys::remove(l_png);
  FSys::remove(l_fake);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
