#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/coreset.h>
#include <DoodleLib/threadPool/ThreadPool.h>

#include <opencv2/opencv.hpp>
#include <PinYIn/convert.h>

#include <boost/format.hpp>
namespace doodle {
std::string ImageSequence::clearString(const std::string &str) {
  auto &con = convert::Get();
  auto str_r = std::string{};
  str_r = con.toEn(str);

  return str_r;
}

ImageSequence::ImageSequence(std::vector<FSys::path> paths, const std::string &text)
    : LongTerm(),
      p_paths(std::move(paths)),
      p_Text(std::move(clearString(text))),
      stride(),
      p_eps(-1),
      p_shot(-1),
      p_shot_ab() {
  for (auto &path : p_paths) {
    if (!FSys::is_regular_file(path)) {
      throw DoodleError("不是文件, 无法识别");
    }
  }
  this->seanInfo();
}

ImageSequence::ImageSequence(const FSys::path &path_dir, const std::string &text)
    : LongTerm(),
      p_paths(),
      p_Text(std::move(clearString(text))),
      stride(),
      p_eps(-1),
      p_shot(-1),
      p_shot_ab() {
  this->seanDir(path_dir);
  this->seanInfo();
  for (auto &path : p_paths) {
    if (!FSys::is_regular_file(path)) {
      throw DoodleError("不是文件, 无法识别");
    }
  }
}

bool ImageSequence::hasSequence() {
  return !p_paths.empty();
}

FSys::path ImageSequence::getDir() const {
  if (!p_paths.empty())
    return p_paths.at(0).parent_path();
  else
    return {};
}

std::tuple<int, int, std::string> ImageSequence::getEpisodesAndShot() {
  return {p_eps, p_shot, p_shot_ab};
}

std::string ImageSequence::getEpisodesAndShot_str() {
  boost::format str_eps{"ep%04d_"};
  boost::format str_shots{"%ssc%04d%s"};
  std::string str{};
  if (p_eps > 0) {
    str_eps % p_eps;
    str = str_eps.str();
  }
  if (p_shot > 0) {
    str_shots % str % p_shot % p_shot_ab;
    str = str_shots.str();
  }

  if (str.empty())
    str = "ep_None_sc_None";

  return str;
  // return {};
}

bool ImageSequence::seanDir(const FSys::path &dir) {
  if (!FSys::is_directory(dir)) throw FileError{dir, "file not is a directory"};

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
  if (p_paths.empty()) throw DoodleError("空目录");
  return true;
}

void ImageSequence::seanInfo() {
  static std::regex k_exp_epis{R"(ep_?(\d+))", std::regex_constants::icase};
  static std::regex k_exp_shot{R"(sc_?(\d+)([a-z])?)", std::regex_constants::icase};
  auto path = p_paths.at(0).generic_string();
  std::smatch k_match{};

  if (std::regex_search(path, k_match, k_exp_epis))
    p_eps = std::stoi(k_match[1].str());
  if (std::regex_search(path, k_match, k_exp_shot)) {
    p_shot = std::stoi(k_match[1].str());
    if (k_match.size() > 2)
      p_shot_ab = k_match[2].str();
  }
}

void ImageSequence::setText(const std::string &text) {
  p_Text = clearString(text);
}

void ImageSequence::createVideoFile(const FSys::path &out_file) {
  if (!this->hasSequence()) throw DoodleError{"not Sequence"};
  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  //检查父路径存在
  if (!FSys::exists(out_file.parent_path()))
    FSys::create_directories(out_file.parent_path());

  const static cv::Size k_size{1280, 720};

  auto video = cv::VideoWriter{out_file.generic_string(),
                               cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                               25,
                               cv::Size(1280, 720)};
  auto k_image = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  auto k_clone = cv::Mat{};

  auto k_size_len = boost::numeric_cast<float>(p_paths.size());
  auto k_i = float{0};
  auto k_format = boost::format{"%s %s :%s"};
  k_format % p_Text % coreSet::getSet().getUser_en() % getEpisodesAndShot_str();
  auto k_Text = k_format.str();

  //排序图片
  std::sort(p_paths.begin(), p_paths.end(),
            [](const FSys::path &k_r, const FSys::path &k_l) -> bool { return k_r.stem() < k_l.stem(); });

  for (auto &&path : p_paths) {
    k_image = cv::imread(path.generic_string());
    if (k_image.empty())
      throw DoodleError("open cv not read image");
    if (k_image.cols != 1280 || k_image.rows != 720)
      cv::resize(k_image, k_image_resized, k_size);
    else
      k_image_resized = k_image;
    int baseLine{};

    {//创建水印
      k_clone = k_image_resized.clone();
      int fontFace = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
      double fontScale = 1;
      int thickness = 2;
      int baseline = 0;
      auto textSize = cv::getTextSize(k_Text, fontFace,
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
      cv::putText(k_image_resized, k_Text, textOrg, fontFace, fontScale,
                  cv::Scalar{0, 255, 255}, thickness, cv::LineTypes::LINE_AA);
    }

    ++k_i;
    this->stride(((float)1 / k_size_len) * (float)100);
    this->progress(boost::numeric_cast<int>((k_i / k_size_len) * 100));

    video << k_image_resized;
  }
  boost::format message{"成功创建视频 %s"};
  message % out_file.generic_string();
  this->messagResult(message.str());
  this->finished();
}

ImageSequenceBatch::ImageSequenceBatch(decltype(p_paths) dirs)
    : LongTerm(),
      p_paths(std::move(dirs)),
      p_imageSequences() {
  for (auto &&dir : p_paths) {
    if (FSys::is_directory(dir))
      p_imageSequences.emplace_back(
          std::make_shared<ImageSequence>(dir));
    else
      throw DoodleError("不是目录");
  }
}

ImageSequenceBatch::ImageSequenceBatch(decltype(p_imageSequences) imageSequences)
    : p_paths(),
      p_imageSequences(std::move(imageSequences)) {
  for (auto &&image : p_imageSequences) {
    p_paths.emplace_back(image->getDir());
  }
}

void ImageSequenceBatch::batchCreateSequence(const FSys::path &out_dir) const {
  if (p_imageSequences.empty()) return;
  auto &set = coreSet::getSet();
  //这里使用序列图的父路径加uuid防止重复
  auto k_path = p_imageSequences[0]->getDir().parent_path() /
                boost::uuids::to_string(set.getUUID());
  // auto k_path = set.getCacheRoot() / boost::uuids::to_string(set.getUUID());

  //创建生成路径
  if (!out_dir.empty())
    k_path = out_dir;
  //检查路径存在
  if (!FSys::exists(k_path))
    FSys::create_directories(k_path);

  //创建线程池, 开始
  ThreadPool thread_pool{std::thread::hardware_concurrency()};
  std::map<FSys::path, std::future<void>> result{};
  //创建锁
  std::mutex p_mutex{};
  auto k_i = float{1};
  //添加进度回调函数
  auto k_add_fun = std::bind<void>([&p_mutex](float i, float *_1) {
    std::unique_lock lock{p_mutex};
    (*_1) += i;
  },
                                   std::placeholders::_1, &k_i);
  for (const auto &im : p_imageSequences) {
    auto str = im->getEpisodesAndShot_str().append(".mp4");
    im->stride.connect(k_add_fun);

    result.emplace(im->getDir(),
                   thread_pool.enqueue(
                       [k_path, str, im] {
                         // !从这里开始送入线程池, 防止线程检查重名式失败
                         //检查存在,如果存在就使用其他名称
                         auto path = k_path / str;
                         if (FSys::exists(path) || str == "ep0000_sc0000") {
                           path.remove_filename();
                           path /= boost::uuids::to_string(coreSet::getSet().getUUID()).append(".mp4");
                         }
                         im->createVideoFile(path);
                       }));
  }
  std::future_status status{};
  auto it = result.begin();
  const auto k_len = boost::numeric_cast<float>(p_imageSequences.size());

  while (!result.empty()) {
    status = it->second.wait_for(std::chrono::milliseconds{10});
    {
      std::unique_lock lock{p_mutex};
      this->progress(boost::numeric_cast<int>(k_i / k_len));
    }
    if (status == std::future_status::ready) {
      ++k_i;

      boost::format k_msg{"%s : %s\n"};
      k_msg % it->first.generic_string();
      try {
        it->second.get();
      } catch (const DoodleError &err) {
        k_msg % std::string{"失败: "}.append(err.what());
      }
      if (k_msg.fed_args() == 1)
        k_msg % "成功";

      this->messagResult(k_msg.str());
      //这里要擦除数据
      it = result.erase(it);
    } else {
      //超时后继续等待其他
      ++it;
    }
    // 如果到了结尾就返回开始
    if (it == result.end()) {
      it = result.begin();
    }
  }
  this->finished();
}

}  // namespace doodle
