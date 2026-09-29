#include <doodle_lib/http_method/kitsu/ai_light_stats.h>

#include <doodle_core/exception/exception.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <string_view>
#include <utility>
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

// ---------------------------------------------------------------------------
// 多图聚合与冲突检测（§5.1 的 ②，即 §5.3）
// ---------------------------------------------------------------------------

/// 相对判据的分母地板：中位数接近 0 时（例如整幅压黑的夜景，p10 = 0），
/// 直接除以中位数会得到 inf。取一个远小于指标量程的常数后，
/// 「相对离散度」在这种情形下退化成「绝对极差 / 0.05」，仍然是单调可比的。
constexpr double g_relative_scale_floor = 0.05;

/// 削波目标的允许余量：单张参考（或所有图都不削波）时极差为 0，
/// 目标会退化成 [0, 0] = 「一个像素都不许削」。给一点余量，
/// 表达的是「无**可见**削波」，而不是「绝对零削波」。
constexpr double g_clip_allowance = 0.01;

/// 提黑判据：显示空间里 p10 明显不为 0，就是黑场被抬起来了。
constexpr double g_lifted_blacks_p10 = 0.12;

/// 判定「参考图被修过」的痕迹强度门槛。单图的 retouch_score_ 本就是下界
/// （只覆盖暗角），所以这里也不拿它做任何门控，只用来填 reference_stats_.retouch_detected_。
constexpr double g_retouch_detected_threshold = 0.4;

/// 调色板占比的冲突阈值（绝对量）。0.15 ≈ 主色占比跨图差出一个「次主色」的量级。
constexpr double g_palette_share_conflict = 0.15;

/// 冲突维度的置信度上限：冲突了就不该给高置信度，但**也不清零** ——
/// 清零等于宣称「没有统计」，那是 prompt_only 降级的意思，
/// 而「有统计但互相矛盾」是另一回事，要交人工裁决（§5.3 第 3~4 步）。
constexpr double g_conflict_confidence_cap = 0.4;

/// 疑似外点的判据：Σ 各维度「离中位数多少个阈值」超过此值就报出来。
/// 取 2.5 意味着**单个维度**离中位数 2.5 个阈值即命中 ——
/// 再高就报不出「其中一张图的色温明显不一样」这种最典型的外点。
constexpr double g_outlier_score_threshold = 2.5;
/// 少于这么多张可用图时不做外点判定：3 张图里挑外点，
/// 等于拿 2 张当基准，随时会把「少数派」误判成「外点」。
constexpr std::size_t g_outlier_min_images = 4;

/// 可聚合标量的下标。**顺序即契约**，与 g_dimension_rules 一一对应。
constexpr std::size_t g_dim_luma_p10  = 0;
constexpr std::size_t g_dim_luma_p50  = 1;
constexpr std::size_t g_dim_luma_p90  = 2;
constexpr std::size_t g_dim_contrast  = 3;
constexpr std::size_t g_dim_kelvin    = 4;
constexpr std::size_t g_dim_clip_high = 5;
constexpr std::size_t g_dim_clip_low  = 6;

using dimension_values_t = std::array<double, 7>;

/// 一个维度的冲突判据 + 零宽区间的展宽容差。
struct dimension_rule_t {
  std::string_view key;
  /// >0：用「离散度 / max(|中位数|, g_relative_scale_floor)」判冲突（尺度无关的维度）
  double conflict_relative{0.0};
  /// >0：用绝对极差判冲突（本身就有物理量纲的维度，如 Kelvin）
  double conflict_absolute{0.0};
  /// 零宽区间时按该相对量展宽
  double single_relative{0.0};
  /// 展宽量的绝对下限（避免相对量为 0 的维度给出 0 宽区间）
  double single_absolute{0.0};
  /// clip_rate：目标是「不超过」，区间恒为 [0, ·]
  bool upper_bound_only{false};
};

/// 阈值表。每个维度一条，避免「每个维度一套 if」散在函数体里。
constexpr std::array<dimension_rule_t, 7> g_dimension_rules{{
    {"luma_percentiles.p10", 0.20, 0.0, 0.10, 0.02, false},
    {"luma_percentiles.p50", 0.20, 0.0, 0.10, 0.02, false},
    {"luma_percentiles.p90", 0.20, 0.0, 0.10, 0.02, false},
    {"contrast", 0.25, 0.0, 0.10, 0.05, false},
    {"white_balance_kelvin", 0.0, 250.0, 0.0, 200.0, false},
    {"clip_rate.high", 0.0, 0.03, 0.0, 0.0, true},
    {"clip_rate.low", 0.0, 0.05, 0.0, 0.0, true},
}};

dimension_values_t to_dimensions(const reference_image_stats_t& in_stats) {
  return dimension_values_t{
      in_stats.luma_percentiles_.p10_,
      in_stats.luma_percentiles_.p50_,
      in_stats.luma_percentiles_.p90_,
      in_stats.contrast_,
      in_stats.white_balance_kelvin_,
      in_stats.clip_rate_.high_,
      in_stats.clip_rate_.low_,
  };
}

/// 中位数。按值收参是为了就地 nth_element —— 调用方多半不再需要原序列。
double median_of(std::vector<double> in_values) {
  if (in_values.empty()) return 0.0;
  const std::size_t l_mid = in_values.size() / 2;
  std::nth_element(in_values.begin(), in_values.begin() + static_cast<std::ptrdiff_t>(l_mid), in_values.end());
  const double l_upper = in_values[l_mid];
  if (in_values.size() % 2 == 1) return l_upper;
  // 偶数个：nth_element 之后 [begin, mid) 全部 <= 中位数，取其中的最大值即下中位数。
  const auto l_lower = std::max_element(in_values.begin(), in_values.begin() + static_cast<std::ptrdiff_t>(l_mid));
  return (*l_lower + l_upper) * 0.5;
}

double dimension_spread(const dimension_rule_t& in_rule, const std::vector<double>& in_values, double in_median) {
  const auto [l_min, l_max] = std::minmax_element(in_values.begin(), in_values.end());
  const double l_range      = *l_max - *l_min;
  if (in_rule.conflict_relative > 0.0)
    return l_range / std::max(std::abs(in_median), g_relative_scale_floor);
  return l_range;
}

/// 区间化。**极差区间就是「多图一致范围」** —— 图之间差异越大区间越宽，
/// 这本身就是「参考是近似而非精确」在数据上的体现；冲突只是额外被标出来，
/// 绝不把区间收窄成平均值（§5.3 第 3 步：不自动平均掉）。
interval_t dimension_interval(const dimension_rule_t& in_rule, const std::vector<double>& in_values) {
  const auto [l_min_it, l_max_it] = std::minmax_element(in_values.begin(), in_values.end());
  const double l_min              = *l_min_it;
  const double l_max              = *l_max_it;

  if (in_rule.upper_bound_only) return interval_t{0.0, std::max(l_max, g_clip_allowance)};
  if (l_max > l_min) return interval_t{l_min, l_max};

  // 单图（或所有图恰好一致）：零宽区间会让评判层的「落点是否在区间内」永远判失败，
  // 所以以该值为中心按容差展开。
  const double l_half = std::max(in_rule.single_relative * std::abs(l_min), in_rule.single_absolute);
  return interval_t{l_min - l_half, l_min + l_half};
}

/// 调色板聚合：把所有可用图的条目汇成点集重新聚类，得到跨图共享的 k 个中心色；
/// 再把**每张图**的条目按最近中心归簇、累加 share，于是每个簇都带一个跨图区间。
///
/// 为什么用 min/max 而不是各图中位数：中位数对「某张图根本没有这个色」不敏感 ——
/// 那个 0 会被中位数吃掉；而 min 会把 0 留成下界，那才是「这个色不是每张图都有」
/// 这个事实的正确表达。
std::vector<palette_entry_t> aggregate_palette(const std::vector<std::vector<image_palette_entry_t>>& in_palettes) {
  // 用扁平 float 缓冲而不是 std::vector<cv::Vec3f>：cv::Mat 的构造要求 3 个 float 严格连续，
  // 扁平数组把这个前提写在脸上，不依赖 cv::Vec3f 的内部布局。
  std::vector<float> l_flat{};
  for (const auto& l_palette : in_palettes) {
    for (const auto& l_entry : l_palette) {
      l_flat.push_back(static_cast<float>(l_entry.color_.x_));
      l_flat.push_back(static_cast<float>(l_entry.color_.y_));
      l_flat.push_back(static_cast<float>(l_entry.color_.z_));
    }
  }
  if (l_flat.empty()) return {};

  const int l_point_count = static_cast<int>(l_flat.size() / 3);
  const cv::Mat l_samples(l_point_count, 3, CV_32F, l_flat.data());

  const int l_k = std::min<int>(g_palette_k, l_point_count);
  cv::Mat l_centers;
  if (l_k <= 1) {
    l_centers = cv::Mat(1, 3, CV_32F);
    for (int c = 0; c < 3; ++c) l_centers.at<float>(0, c) = l_flat[static_cast<std::size_t>(c)];
  } else {
    // 与单图调色板同一个理由：固定种子。否则同一批参考图会算出不同的调色板目标，
    // Spec 的版本对比与单测都无从解释。
    cv::setRNGSeed(0);
    cv::Mat l_labels;
    cv::kmeans(
        l_samples, l_k, l_labels, cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 10, 1.0), 3,
        cv::KMEANS_PP_CENTERS, l_centers
    );
  }

  // 每张图在每个簇上的占比。用「最近中心」而不是每张图重跑一次 kmeans：
  // 图各跑一次会得到各自不同的簇编号，占比就没法对齐到同一个目标色上。
  std::vector<std::vector<double>> l_per_image(
      in_palettes.size(), std::vector<double>(static_cast<std::size_t>(l_centers.rows), 0.0)
  );
  for (std::size_t i = 0; i < in_palettes.size(); ++i) {
    for (const auto& l_entry : in_palettes[i]) {
      const cv::Vec3f l_point{
          static_cast<float>(l_entry.color_.x_), static_cast<float>(l_entry.color_.y_),
          static_cast<float>(l_entry.color_.z_)
      };
      int l_best             = 0;
      double l_best_distance = std::numeric_limits<double>::max();
      for (int c = 0; c < l_centers.rows; ++c) {
        const cv::Vec3f l_center{l_centers.at<float>(c, 0), l_centers.at<float>(c, 1), l_centers.at<float>(c, 2)};
        const double l_distance = cv::norm(l_point - l_center);
        if (l_distance < l_best_distance) {
          l_best_distance = l_distance;
          l_best          = c;
        }
      }
      l_per_image[i][static_cast<std::size_t>(l_best)] += l_entry.share_;
    }
  }

  std::vector<palette_entry_t> l_ret{};
  l_ret.reserve(static_cast<std::size_t>(l_centers.rows));
  for (int c = 0; c < l_centers.rows; ++c) {
    palette_entry_t l_entry{};
    // 中心就是 RGB 序（点集按 palette_entry_t::color_ 的 RGB 序建），不需要换序。
    l_entry.color_ = vec3_t{
        std::clamp(static_cast<double>(l_centers.at<float>(c, 0)), 0.0, 1.0),
        std::clamp(static_cast<double>(l_centers.at<float>(c, 1)), 0.0, 1.0),
        std::clamp(static_cast<double>(l_centers.at<float>(c, 2)), 0.0, 1.0),
    };
    double l_min = 1.0;
    double l_max = 0.0;
    for (const auto& l_shares : l_per_image) {
      l_min = std::min(l_min, l_shares[static_cast<std::size_t>(c)]);
      l_max = std::max(l_max, l_shares[static_cast<std::size_t>(c)]);
    }
    l_entry.share_ = interval_t{l_min, l_max};
    l_ret.push_back(l_entry);
  }
  // 按区间中心降序：与单图 extract_palette 的「按占比降序」一致，
  // 这样「palette[0] 是主色」这条读法在两层都成立。
  std::sort(l_ret.begin(), l_ret.end(), [](const auto& in_a, const auto& in_b) {
    return in_a.share_.center() > in_b.share_.center();
  });
  return l_ret;
}

/// 调色倾向：由统计中位数分档。产出的是**可读标签**而不是数值 ——
/// grading_hints_t 的契约就是如此（具体数值由 §9.2 全局调色反解去求）。
grading_hints_t derive_grading_hints(const dimension_values_t& in_medians, double in_median_retouch) {
  grading_hints_t l_hints{};
  // 暗角：直接复用单图的痕迹强度。它本来就只覆盖暗角这一类签名，是个下界，
  // 所以这里读到的值只会**低估**痕迹，不会凭空造出暗角。
  l_hints.vignette_ = in_median_retouch >= 0.70   ? "strong"
                      : in_median_retouch >= 0.45 ? "moderate"
                      : in_median_retouch >= 0.20 ? "subtle"
                                                  : "none";
  l_hints.lifted_blacks_ = in_medians[g_dim_luma_p10] > g_lifted_blacks_p10;
  // 泛光：用高光削波率当代理 —— 高光成片溢出正是 bloom 的成因与可见后果。
  const double l_clip_high = in_medians[g_dim_clip_high];
  l_hints.bloom_           = l_clip_high > 0.05 ? "moderate" : l_clip_high > 0.02 ? "subtle" : "none";
  return l_hints;
}

/// 责任归属的默认划分。**本里程碑不由 LLM 决定** ——「不许改几何/材质」是一条产品边界
/// （§6.6），不该让模型每轮重新发明一遍。它决定的是「这个目标该由灯光还是调色去达成」。
ownership_t default_ownership() {
  ownership_t l_ret{};
  l_ret.lighting_ = {"intensity", "direction", "hardness", "color_temperature", "light_count"};
  l_ret.grading_  = {"contrast", "saturation", "vignette", "lifted_blacks", "bloom"};
  l_ret.shared_   = {"white_balance"};
  return l_ret;
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

aggregated_reference_stats_t aggregate_reference_stats(const std::vector<ai_light_reference>& in_references) {
  aggregated_reference_stats_t l_ret{};
  l_ret.stats_.image_count_               = static_cast<std::int32_t>(in_references.size());
  l_ret.stats_.aggregation_               = "median";
  l_ret.stats_.usable_for_physical_solve_ = false;

  // ── ① 逐图提取：可用图筛选 ────────────────────────────────────────────────
  // 三条判据缺一不可：人工排除过 / 置信度为 0（统计层没跑过）/ stats 列还是 null。
  std::vector<dimension_values_t> l_dimensions{};
  std::vector<std::vector<image_palette_entry_t>> l_palettes{};
  std::vector<double> l_confidences{};
  std::vector<double> l_retouch{};
  std::vector<uuid> l_ids{};
  for (const auto& l_ref : in_references) {
    if (l_ref.is_outlier_) continue;
    if (!(l_ref.stats_confidence_ > 0.0)) continue;
    if (l_ref.stats_.is_null()) continue;
    const auto l_stats = l_ref.get_stats();
    l_dimensions.push_back(to_dimensions(l_stats));
    l_palettes.push_back(l_stats.palette_);
    l_confidences.push_back(l_ref.stats_confidence_);
    l_retouch.push_back(l_stats.retouch_score_);
    l_ids.push_back(l_ref.uuid_id_);
  }

  if (l_dimensions.empty()) {
    // 「没有可用统计」不是错误：Spec 仍然出得来，只是降级成 prompt_only。
    // 降级判定留给调用方（它看 per_dimension_confidence_ 是否为空），
    // 因为「谁有权声称降级」应该是接缝、而不是这个纯函数。
    l_ret.stats_.conflicts_.push_back("no_usable_stats");
    l_ret.targets_ = lighting_targets_t{};
    return l_ret;
  }

  const double l_mean_confidence =
      std::accumulate(l_confidences.begin(), l_confidences.end(), 0.0) / static_cast<double>(l_confidences.size());

  // ── ② 逐维度：中位数 → 区间 → 冲突 ────────────────────────────────────────
  dimension_values_t l_medians{};
  std::array<interval_t, 7> l_intervals{};
  for (std::size_t d = 0; d < g_dimension_rules.size(); ++d) {
    const auto& l_rule = g_dimension_rules[d];
    std::vector<double> l_values{};
    l_values.reserve(l_dimensions.size());
    for (const auto& l_row : l_dimensions) l_values.push_back(l_row[d]);

    l_medians[d]   = median_of(l_values);
    l_intervals[d] = dimension_interval(l_rule, l_values);

    const double l_spread    = dimension_spread(l_rule, l_values, l_medians[d]);
    const double l_threshold = l_rule.conflict_relative > 0.0 ? l_rule.conflict_relative : l_rule.conflict_absolute;
    const std::string l_key{l_rule.key};

    // 单值维度（所有图恰好一致）离散度恒为 0，天然不冲突，无需特判。
    const bool l_conflict = l_threshold > 0.0 && l_spread > l_threshold;

    l_ret.dimension_spread_[l_key] = l_spread;
    if (l_conflict) l_ret.stats_.conflicts_.push_back(l_key);

    // 未冲突：按「离阈值还有多远」线性打折，再乘各图置信度的均值。
    // 冲突：压到上限 —— 不清零，因为清零的语义是「没有统计」，而这里是「统计互相矛盾」。
    double l_confidence = l_mean_confidence;
    if (l_threshold > 0.0) l_confidence *= std::max(0.0, 1.0 - l_spread / l_threshold);
    if (l_conflict) l_confidence = std::min(l_confidence, g_conflict_confidence_cap);
    l_ret.stats_.per_dimension_confidence_[l_key] = std::clamp(l_confidence, g_confidence_floor, 1.0);
  }

  // ── ③ 组装目标 ───────────────────────────────────────────────────────────
  // 痕迹强度中位数要算两次用途（分档填 vignette_、判定 retouch_detected_），先算一次。
  const double l_median_retouch = median_of(l_retouch);

  lighting_targets_t l_targets{};
  l_targets.space_                 = "display_srgb";
  l_targets.luma_percentiles_.p10_ = l_intervals[g_dim_luma_p10];
  l_targets.luma_percentiles_.p50_ = l_intervals[g_dim_luma_p50];
  l_targets.luma_percentiles_.p90_ = l_intervals[g_dim_luma_p90];
  l_targets.contrast_              = l_intervals[g_dim_contrast];
  l_targets.white_balance_kelvin_  = l_intervals[g_dim_kelvin];
  l_targets.clip_rate_.high_       = l_intervals[g_dim_clip_high];
  l_targets.clip_rate_.low_        = l_intervals[g_dim_clip_low];
  l_targets.palette_               = aggregate_palette(l_palettes);
  l_targets.grading_hints_         = derive_grading_hints(l_medians, l_median_retouch);
  l_targets.ownership_             = default_ownership();

  // 调色板占比的冲突也要报：主色占比是「全局调色为主」的参考里最容易被改动的量，
  // 几张图占比差得远，意味着「主色到底是什么」本身有分歧。
  for (std::size_t p = 0; p < l_targets.palette_.size(); ++p) {
    const double l_spread   = l_targets.palette_[p].share_.width();
    const std::string l_key = "palette[" + std::to_string(p) + "].share";
    l_ret.dimension_spread_[l_key] = l_spread;
    if (l_spread > g_palette_share_conflict) {
      l_ret.stats_.conflicts_.push_back(l_key);
      l_ret.stats_.per_dimension_confidence_[l_key] = g_conflict_confidence_cap;
    } else {
      l_ret.stats_.per_dimension_confidence_[l_key] =
          std::clamp(l_mean_confidence * (1.0 - l_spread / g_palette_share_conflict), g_confidence_floor, 1.0);
    }
  }

  l_ret.stats_.retouch_detected_ = median_of(l_retouch) > g_retouch_detected_threshold;
  l_ret.targets_                 = std::move(l_targets);

  // ── ④ 疑似外点（只报不写）──────────────────────────────────────────────
  // 判据是「Σ 各维度离开中位数多少个阈值」。用阈值（而不是标准差）当尺度，
  // 是为了让 7 个量纲完全不同的维度可以直接相加：色温的 250K 与对比度的 0.25
  // 在各自维度里是同一个「一格」。
  if (l_dimensions.size() >= g_outlier_min_images) {
    for (std::size_t i = 0; i < l_dimensions.size(); ++i) {
      double l_score = 0.0;
      for (std::size_t d = 0; d < g_dimension_rules.size(); ++d) {
        const auto& l_rule = g_dimension_rules[d];
        const double l_scale =
            l_rule.conflict_relative > 0.0
                ? l_rule.conflict_relative * std::max(std::abs(l_medians[d]), g_relative_scale_floor)
                : l_rule.conflict_absolute;
        if (l_scale <= 0.0) continue;
        l_score += std::abs(l_dimensions[i][d] - l_medians[d]) / l_scale;
      }
      if (l_score > g_outlier_score_threshold) l_ret.suspected_outlier_ids_.push_back(l_ids[i]);
    }
  }

  return l_ret;
}

}  // namespace doodle::ai_light
