#include <doodle_lib/http_method/kitsu/ai_light_stats.h>

#include <doodle_core/exception/exception.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

namespace doodle::ai_light {
namespace {

/// 调色板簇数。5 个主色足够表达「主色 / 补色 / 环境色 / 高光 / 暗部」，
/// 同时让 Spec 与喂给 LLM 的提示词保持可读。k 越大占比越碎，反而不利于当目标用。
constexpr int g_palette_k = 5;
/// k-means 的代价与像素数成正比，而调色板只需要「主色是什么」。
/// 缩到 256 边长后样本量 ≤ 65536，占比的统计误差已在 1% 量级 —— 远小于后面 1% 的丢簇阈值。
constexpr int g_palette_max_side = 256;
/// share 低于此值的尾部簇不是主色，只是噪声
constexpr double g_palette_min_share = 0.01;
/// 但至少保留这么多个：簇太少的话「分布」就不成其为分布了
constexpr std::size_t g_palette_min_keep = 3;

/// 参考图缺失统计时的中性默认（D65）。用在「色度偏得离谱、公式没有意义」的兜底上，
/// 而不是给一个假的高精度数字。
constexpr double g_kelvin_fallback = 6500.0;
constexpr double g_kelvin_min      = 1500.0;
constexpr double g_kelvin_max      = 15000.0;

/// 分辨率基准：不到 720p 的图按线性比例打折（1280×720 → 因子 1.0）
constexpr double g_resolution_baseline_pixels = 1280.0 * 720.0;
/// 几乎纯色的图（p90 − p10 小于此值）几乎没有可迁移的信息
constexpr double g_flat_image_contrast = 0.02;
constexpr double g_flat_image_factor   = 0.1;
/// 后处理痕迹对置信度的最大折扣系数：满分的痕迹也只打对折，
/// 因为参考图**本来就该是修过的**（§5.2 把这类定性痕迹的稳健性定为「中」）
constexpr double g_retouch_confidence_weight = 0.5;
constexpr double g_confidence_floor          = 0.05;

/// 暗角落差达到这个幅度就算满分痕迹：边缘中点亮度掉到中心的 75%
constexpr double g_vignette_full_drop = 0.25;

/// 4 通道以外、非 8bit 的输入都直接抛。这里刻意不「顺手转一下」：
/// 8bit 与 32F 的数值域差了 255 倍，猜错会让所有指标静默偏掉 255 倍。
void check_supported(const cv::Mat& in_image) {
  if (in_image.empty() || in_image.rows <= 0 || in_image.cols <= 0)
    throw_exception(doodle_error{"参考图统计：空图像"});
  if (in_image.depth() != CV_8U)
    throw_exception(doodle_error{"参考图统计：只接受 8bit 图像，收到 depth={}", static_cast<int>(in_image.depth())});
  const int l_channels = in_image.channels();
  if (l_channels != 1 && l_channels != 3 && l_channels != 4)
    throw_exception(doodle_error{"参考图统计：只接受 1/3/4 通道图像，收到 {}", l_channels});
}

/// 归一到 3 通道 8bit BGR。灰度与带 alpha 的图在这里被统一 ——
/// imread(IMREAD_COLOR) 本来就只给 3 通道，这条分支是给直接调本函数的调用方（含单测）用的。
cv::Mat to_bgr8(const cv::Mat& in_image) {
  switch (in_image.channels()) {
    case 1: {
      cv::Mat l_out;
      cv::cvtColor(in_image, l_out, cv::COLOR_GRAY2BGR);
      return l_out;
    }
    case 4: {
      cv::Mat l_out;
      cv::cvtColor(in_image, l_out, cv::COLOR_BGRA2BGR);
      return l_out;
    }
    default: return in_image;
  }
}

/// 归一到 [0,1] 的 3 通道 BGR (CV_32F)。
cv::Mat to_unit_bgr_f32(const cv::Mat& in_bgr8) {
  cv::Mat l_unit;
  in_bgr8.convertTo(l_unit, CV_32F, 1.0 / 255.0);
  return l_unit;
}

/// 显示空间亮度：在 sRGB **编码值**上按 Rec.709 权重加权。
///
/// 这是 display-space 的 luma（不是线性光的 luminance）—— §8.2 要的就是这个量。
/// 权重按 BGR 序排列，与 cv::imread 的通道序一致。
cv::Mat display_luma(const cv::Mat& in_unit_bgr) {
  static const cv::Mat l_weights = (cv::Mat_<float>(1, 3) << 0.0722f, 0.7152f, 0.2126f);
  cv::Mat l_luma;
  cv::transform(in_unit_bgr, l_luma, l_weights);
  return l_luma;  // CV_32F, 0..1
}

/// 256 bin 亮度直方图，bin i 就是亮度等级 i。
///
/// 为什么先量化到 8bit 再统计，而不是拿浮点亮度直接喂 calcHist：
/// calcHist 对浮点输入用的是 **[low, high) 半开区间**，值恰好等于 1.0 的像素会被丢掉 ——
/// 整幅纯白、或成片过曝白都会命中这种情况，于是直方图的样本总数不等于像素总数，
/// 纯白图的三个分位会一起变成 0（对照度/置信度的判断就全错了）。
/// 量化到 256 个等级后 bin 与等级一一对应，等级 0 与 255 都是闭的。
cv::Mat luma_histogram(const cv::Mat& in_luma) {
  cv::Mat l_luma8;
  in_luma.convertTo(l_luma8, CV_8U, 255.0);

  cv::Mat l_hist;
  int l_channels[]          = {0};
  int l_hist_size[]         = {256};
  float l_ranges[]          = {0.0f, 256.0f};
  const float* l_ranges_p[] = {l_ranges};
  cv::calcHist(&l_luma8, 1, l_channels, cv::noArray(), l_hist, 1, l_hist_size, l_ranges_p);
  return l_hist;
}

/// 从直方图取分位，取值就是等级本身（i/255）。
/// 量化误差 ±0.5/255 ≈ 0.002 —— 而 Spec 的区间宽度是 0.2 量级
/// （lighting_targets_t 默认 contrast 就是 [0.55, 0.75]），
/// 所以这个误差对「落点在不在区间内」的判断没有影响。
/// 副产品是纯黑恰好得 0、纯白恰好得 1，没有「全在 bin 0 却是 0.002」这种别扭值。
double percentile_of(const cv::Mat& in_hist, double in_quantile) {
  // 用直方图而不是排序：像素数无上限，O(n) 累积与分辨率无关。
  const double l_total = static_cast<double>(cv::sum(in_hist)[0]);
  if (l_total <= 0.0) return 0.0;
  const double l_target = in_quantile * l_total;
  double l_cum          = 0.0;
  for (int i = 0; i < in_hist.rows; ++i) {
    l_cum += static_cast<double>(in_hist.at<float>(i));
    if (l_cum >= l_target) return static_cast<double>(i) / 255.0;
  }
  return 1.0;
}

/// 调色板：k-means（显示空间）→ 按占比降序 → 丢掉噪声尾部簇 → 占比重新归一化。
std::vector<image_palette_entry_t> extract_palette(const cv::Mat& in_unit_bgr) {
  cv::Mat l_small;
  const double l_scale =
      std::min(1.0, static_cast<double>(g_palette_max_side) / static_cast<double>(std::max(in_unit_bgr.cols, in_unit_bgr.rows)));
  if (l_scale < 1.0)
    cv::resize(in_unit_bgr, l_small, cv::Size{}, l_scale, l_scale, cv::INTER_AREA);
  else
    l_small = in_unit_bgr;
  // reshape 要求连续内存；resize 与 convertTo 都给连续图，这里只是防御。
  if (!l_small.isContinuous()) l_small = l_small.clone();

  const cv::Mat l_samples = l_small.reshape(1, static_cast<int>(l_small.total()));

  // **固定随机种子**：cv::theRNG() 是线程局部的，不设种子时 k-means++ 的初始化每次不同，
  // 同一张图会算出不同的调色板。而「同一输入 → 同一结果」是这里必须守住的：
  // 统计一旦不确定，Spec 的版本对比、单测、以及「为什么这一版和上一版目标不一样」都无从解释。
  cv::setRNGSeed(0);
  cv::Mat l_labels, l_centers;
  cv::kmeans(
      l_samples, g_palette_k, l_labels,
      cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, l_centers
  );

  const double l_total = static_cast<double>(l_samples.rows);
  std::vector<image_palette_entry_t> l_entries{};
  l_entries.reserve(static_cast<std::size_t>(l_centers.rows));
  for (int i = 0; i < l_centers.rows; ++i) {
    image_palette_entry_t l_entry{};
    // 中心是 BGR 序（与输入一致），契约里的 vec3_t 是 RGB 序 —— 必须换序，
    // 否则红蓝互换，色板目标整个反掉。
    l_entry.color_ = vec3_t{
        std::clamp(static_cast<double>(l_centers.at<float>(i, 2)), 0.0, 1.0),
        std::clamp(static_cast<double>(l_centers.at<float>(i, 1)), 0.0, 1.0),
        std::clamp(static_cast<double>(l_centers.at<float>(i, 0)), 0.0, 1.0)
    };
    l_entry.share_ = static_cast<double>(cv::countNonZero(l_labels == i)) / l_total;
    l_entries.push_back(l_entry);
  }
  std::sort(l_entries.begin(), l_entries.end(), [](const auto& in_a, const auto& in_b) {
    return in_a.share_ > in_b.share_;
  });
  while (l_entries.size() > g_palette_min_keep && l_entries.back().share_ < g_palette_min_share)
    l_entries.pop_back();

  // 丢簇之后占比之和不再是 1，重新归一化 —— 下游是拿它当「分布」用的。
  const double l_sum = std::accumulate(
      l_entries.begin(), l_entries.end(), 0.0, [](double in_acc, const auto& in_e) { return in_acc + in_e.share_; }
  );
  if (l_sum > 0.0)
    for (auto& l_entry : l_entries) l_entry.share_ /= l_sum;
  return l_entries;
}

/// 削波率：**任一通道**满载/全黑就算该像素被削。
/// 用亮度阈值而不是逐通道，会让夜景、月面这类画面恒为 0 —— 而它们恰恰是削波最需要提示的场景。
clip_rate_value_t compute_clip_rate(const cv::Mat& in_bgr8) {
  std::vector<cv::Mat> l_channels;
  cv::split(in_bgr8, l_channels);

  cv::Mat l_high, l_low, l_tmp;
  for (const auto& l_channel : l_channels) {
    cv::compare(l_channel, 255, l_tmp, cv::CMP_EQ);
    l_high = l_high.empty() ? l_tmp.clone() : (l_high | l_tmp);
    cv::compare(l_channel, 0, l_tmp, cv::CMP_EQ);
    l_low = l_low.empty() ? l_tmp.clone() : (l_low | l_tmp);
  }
  const double l_pixels = static_cast<double>(in_bgr8.total());
  return clip_rate_value_t{
      static_cast<double>(cv::countNonZero(l_high)) / l_pixels,
      static_cast<double>(cv::countNonZero(l_low)) / l_pixels
  };
}

double srgb_to_linear(double in_value) {
  return in_value <= 0.04045 ? in_value / 12.92 : std::pow((in_value + 0.055) / 1.055, 2.4);
}

/// 色温倾向：整幅的色度均值 → McCamy 近似。
///
/// 为什么这一个字段要线性化：Kelvin 是**线性光**色度坐标上的物理尺度。
/// 直接在 sRGB 编码值上算，结果会偏几百 K —— 而 §5.2 的色温区间只有 ~800K 宽
/// （lighting_targets_t 默认是 [3400, 4200]），偏差会直接把「落点」判错。
/// 这是 §8.2「主指标在显示空间计算」的**唯一例外**，已在方案里写明。
///
/// 为什么不做灰世界归一化：那正是要测的东西。「色暖/色冷」就是整幅的色偏倾向，
/// 归一化会把信号一起抹掉。
double estimate_kelvin(const cv::Mat& in_bgr8) {
  // 削波像素会把均值拉向两头，先用「三通道都在 [1, 254]」的掩码排掉它们。
  std::vector<cv::Mat> l_channels;
  cv::split(in_bgr8, l_channels);
  cv::Mat l_mask, l_tmp;
  for (const auto& l_channel : l_channels) {
    cv::Mat l_lo, l_hi;
    cv::compare(l_channel, 1, l_lo, cv::CMP_GE);
    cv::compare(l_channel, 254, l_hi, cv::CMP_LE);
    l_tmp  = l_lo & l_hi;
    l_mask = l_mask.empty() ? l_tmp.clone() : (l_mask & l_tmp);
  }
  const bool l_use_mask = cv::countNonZero(l_mask) > 0;

  // 用 256 项查找表线性化，并且**逐通道**做 —— cv::LUT 的通道语义在单通道入/单通道出时
  // 毫无歧义，多通道那条路径则依赖「LUT 单通道时对每个通道复用」这个细节，不值得赌。
  cv::Mat l_lut(1, 256, CV_32F);
  for (int i = 0; i < 256; ++i) l_lut.at<float>(0, i) = static_cast<float>(srgb_to_linear(i / 255.0));
  std::vector<cv::Mat> l_linear_channels;
  l_linear_channels.reserve(l_channels.size());
  for (const auto& l_channel : l_channels) {
    cv::Mat l_linear_channel;
    cv::LUT(l_channel, l_lut, l_linear_channel);
    l_linear_channels.push_back(l_linear_channel);
  }
  cv::Mat l_linear;
  cv::merge(l_linear_channels, l_linear);  // 3 通道 CV_32F，线性光

  // 先线性化再求均值。反过来（先求均值再线性化）是错的：均值与非线性变换不可交换。
  const cv::Scalar l_mean = l_use_mask ? cv::mean(l_linear, l_mask) : cv::mean(l_linear);
  const double l_r        = l_mean[2];
  const double l_g        = l_mean[1];
  const double l_b        = l_mean[0];

  // sRGB D65 → CIE XYZ
  const double l_x   = 0.4124564 * l_r + 0.3575761 * l_g + 0.1804375 * l_b;
  const double l_y   = 0.2126729 * l_r + 0.7151522 * l_g + 0.0721750 * l_b;
  const double l_z   = 0.0193339 * l_r + 0.1191920 * l_g + 0.9503041 * l_b;
  const double l_sum = l_x + l_y + l_z;
  if (l_sum <= 1e-9) return g_kelvin_fallback;  // 纯黑

  const double l_cx = l_x / l_sum;
  const double l_cy = l_y / l_sum;

  // McCamy 只在**普朗克轨迹附近**有意义。色度偏得离谱的图（纯色块、浓绿植被、
  // 很强的单色光）会算出一个看着精确、实际没有意义的数字，这时回落到 D65 中性 ——
  // 一个诚实的「不知道」，而不是一个假的精确值。
  if (l_cx < 0.20 || l_cx > 0.55 || l_cy < 0.20 || l_cy > 0.45) return g_kelvin_fallback;
  const double l_denominator = 0.1858 - l_cy;
  if (std::abs(l_denominator) < 1e-6) return g_kelvin_fallback;

  const double l_n   = (l_cx - 0.3320) / l_denominator;
  const double l_cct = 449.0 * l_n * l_n * l_n + 3525.0 * l_n * l_n + 6823.3 * l_n + 5520.33;
  if (!std::isfinite(l_cct)) return g_kelvin_fallback;
  return std::clamp(l_cct, g_kelvin_min, g_kelvin_max);
}

/// 后处理痕迹强度（0..1）。本轮只实现**暗角**这一类签名 —— 所以它是一个**下界**：
/// 光晕、局部重绘、频率域痕迹需要多尺度分析，未实现。
///
/// 判据是「**四条边的中点**都比中心暗」。用边的中点而不是整条外缘带宽，是为了把
/// 「画面某一边被压暗」这类**内容**排除掉：整条带会在角上互相重叠，一条边偏暗会
/// 连带把相邻两条边也拉低，于是任何单侧压暗都会被误判成暗角。
///
/// **已知的混淆因素**：中央亮、四周暗的**布光**（暗环境里打亮主角）与暗角给出同一个签名，
/// 单张图无法区分（§5.2 本来就把这类痕迹的稳健性定为「中」，§5.1 把受光方向定为「低」）。
/// 它只按 g_retouch_confidence_weight 影响置信度、不做任何门控，所以混淆的代价被限幅。
double vignette_score(const cv::Mat& in_luma) {
  const int l_rows = in_luma.rows;
  const int l_cols = in_luma.cols;
  const int l_band_h = std::max(1, static_cast<int>(static_cast<double>(l_rows) * 0.10));
  const int l_band_w = std::max(1, static_cast<int>(static_cast<double>(l_cols) * 0.10));

  const cv::Rect l_center(
      l_cols / 4, l_rows / 4, std::max(1, l_cols / 2), std::max(1, l_rows / 2)
  );
  const double l_center_mean = cv::mean(in_luma(l_center))[0];
  if (l_center_mean < 0.02) return 0.0;  // 中心本来就黑，比值没有意义

  const int l_mid_x = std::max(1, l_cols / 5);
  const int l_mid_y = std::max(1, l_rows / 5);
  const std::array<cv::Rect, 4> l_bands{
      cv::Rect(l_cols / 2 - l_mid_x / 2, 0, l_mid_x, l_band_h),
      cv::Rect(l_cols / 2 - l_mid_x / 2, l_rows - l_band_h, l_mid_x, l_band_h),
      cv::Rect(0, l_rows / 2 - l_mid_y / 2, l_band_w, l_mid_y),
      cv::Rect(l_cols - l_band_w, l_rows / 2 - l_mid_y / 2, l_band_w, l_mid_y),
  };

  double l_ratio_sum = 0.0;
  for (const auto& l_band : l_bands) {
    const double l_ratio = cv::mean(in_luma(l_band))[0] / l_center_mean;
    // 有一条边不比中心暗（或更亮）就不是径向衰减，而是画面内容
    if (l_ratio >= 1.0) return 0.0;
    l_ratio_sum += l_ratio;
  }
  return std::clamp((1.0 - l_ratio_sum / 4.0) / g_vignette_full_drop, 0.0, 1.0);
}

/// 该图的统计有多可信（0.05..1.0）。
///
/// 三个因子相乘：分辨率（不到 720p 线性打折）、后处理痕迹、内容是否退化。
/// 看的是「这张图的统计能不能当**目标**用」，而不是「这张图好不好」。
double compute_confidence(int in_width, int in_height, double in_contrast, double in_retouch) {
  const double l_pixels = static_cast<double>(in_width) * static_cast<double>(in_height);
  const double l_resolution_factor =
      l_pixels <= 0.0 ? 0.0 : std::min(1.0, std::sqrt(l_pixels / g_resolution_baseline_pixels));
  const double l_content_factor = in_contrast < g_flat_image_contrast ? g_flat_image_factor : 1.0;

  const double l_confidence =
      l_resolution_factor * (1.0 - g_retouch_confidence_weight * in_retouch) * l_content_factor;
  // 地板 0.05：分析成功就不该被判成「没有统计」。纯色块必须走「不崩 + 低置信度」
  // （§8.5 第 4 条），而不是伪装成「统计层没落地」的 prompt_only 降级。
  return std::clamp(l_confidence, g_confidence_floor, 1.0);
}

}  // namespace

reference_analysis_t analyze_display_image(const cv::Mat& in_display_bgr) {
  check_supported(in_display_bgr);
  const cv::Mat l_bgr8 = to_bgr8(in_display_bgr);

  const cv::Mat l_unit = to_unit_bgr_f32(l_bgr8);
  const cv::Mat l_luma = display_luma(l_unit);
  const cv::Mat l_hist = luma_histogram(l_luma);

  reference_image_stats_t l_stats{};
  l_stats.space_ = "display_srgb";
  const double l_p10 = percentile_of(l_hist, 0.10);
  const double l_p50 = percentile_of(l_hist, 0.50);
  const double l_p90 = percentile_of(l_hist, 0.90);
  l_stats.luma_percentiles_ = luma_percentiles_value_t{l_p10, l_p50, l_p90};
  // 对比度用**分位差**而不是标准差：锁定的默认目标
  // lighting_targets_t::contrast_{0.55, 0.75} 只在分位差下成立 ——
  // 同一画面的 p90 − p10 典型是 0.55~0.8，而标准差只有 0.15~0.25，
  // 用标准差会让默认区间把每张图都判成「对比不足」。§5.2 的措辞本就是「分位差或标准差」。
  l_stats.contrast_             = l_p90 - l_p10;
  l_stats.palette_              = extract_palette(l_unit);
  l_stats.white_balance_kelvin_ = estimate_kelvin(l_bgr8);
  l_stats.clip_rate_            = compute_clip_rate(l_bgr8);
  l_stats.retouch_score_        = vignette_score(l_luma);

  reference_analysis_t l_ret{};
  l_ret.stats_      = l_stats;
  l_ret.confidence_ = compute_confidence(l_bgr8.cols, l_bgr8.rows, l_stats.contrast_, l_stats.retouch_score_);
  return l_ret;
}

std::optional<reference_analysis_t> analyze_reference_file(const FSys::path& in_png) {
  if (!FSys::exists(in_png)) return std::nullopt;
  const cv::Mat l_image = cv::imread(in_png.generic_string(), cv::IMREAD_COLOR);
  if (l_image.empty()) return std::nullopt;
  return analyze_display_image(l_image);
}

}  // namespace doodle::ai_light
