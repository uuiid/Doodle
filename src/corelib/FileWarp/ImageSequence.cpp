#include <corelib/FileWarp/ImageSequence.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/coreset.h>
#include <corelib/threadPool/ThreadPool.h>

#include <opencv2/opencv.hpp>
#include <pinyinlib/convert.h>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/format.hpp>
namespace doodle {
std::string ImageSequence::clearString(const std::string &str) {
  auto &con  = dopinyin::convert::Get();
  auto str_r = std::string{};
  str_r      = con.toEn(str);

  return str_r;
}

ImageSequence::ImageSequence(decltype(p_paths) paths, decltype(p_Text) text)
    : p_paths(std::move(paths)),
      p_Text(std::move(clearString(text))),
      progress(),
      messagResult(),
      finished(),
      p_eps(-1),
      p_shot(-1),
      p_shot_ab() {
  this->seanInfo();
}

ImageSequence::ImageSequence(FSys::path path_dir, decltype(p_Text) text)
    : p_paths(),
      p_Text(std::move(clearString(text))),
      progress(),
      messagResult(),
      finished(),
      p_eps(-1),
      p_shot(-1),
      p_shot_ab() {
  this->seanDir(path_dir);
  this->seanInfo();
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
  if (!this->hasSequence()) throw std::runtime_error{"not Sequence"};
  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  //检查父路径存在
  if (!FSys::exists(out_file.parent_path()))
    FSys::create_directories(out_file.parent_path());

  const static cv::Size k_size{1280, 720};

  auto video           = cv::VideoWriter{out_file.generic_string(),
                               cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                               25,
                               cv::Size(1280, 720)};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};

  auto k_size_len = boost::numeric_cast<float>(p_paths.size());
  auto k_i        = float{0};
  auto k_format   = boost::format{"%s %s :%s"};
  k_format % p_Text % coreSet::getSet().getUser_en() % getEpisodesAndShot_str();
  auto k_Text = k_format.str();

  for (auto &&path : p_paths) {
    k_image = cv::imread(path.generic_string());
    if (k_image.empty())
      throw std::runtime_error("open cv not read image");
    if (k_image.cols != 1280 || k_image.rows != 720)
      cv::resize(k_image, k_image_resized, k_size);
    else
      k_image_resized = k_image;

    cv::putText(k_image_resized, k_Text, cv::Point{30, 50}, cv::HersheyFonts::FONT_HERSHEY_TRIPLEX, double{1}, cv::Scalar{0, 0, 1});
    ++k_i;
    this->progress(boost::numeric_cast<int>((k_i / k_size_len) * 100));

    video << k_image_resized;
  }
  boost::format message{"成功创建视频 %s"};
  message % out_file.generic_string();
  this->messagResult(message.str());
  this->finished();
}

ImageSequenceBatch::ImageSequenceBatch(decltype(p_paths) dirs)
    : p_paths(std::move(dirs)),
      p_imageSequences() {
  for (auto &&dir : p_paths) {
    if (FSys::is_directory(dir))
      p_imageSequences.emplace_back(
          std::make_shared<ImageSequence>(dir));
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
  auto &set   = coreSet::getSet();
  auto k_path = set.getCacheRoot() / boost::uuids::to_string(set.getUUID());

  //创建生成路径
  if (!out_dir.empty())
    k_path = out_dir;
  //检查路径存在
  if (!FSys::exists(k_path))
    FSys::create_directories(k_path);

  //创建线程池, 开始
  ThreadPool thread_pool{std::thread::hardware_concurrency()};
  std::map<FSys::path, std::future<void>> result{};

  for (auto im : p_imageSequences) {
    auto str = im->getEpisodesAndShot_str().append(".mp4");

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
  auto status      = std::future_status{};
  auto it          = result.begin();
  const auto k_len = boost::numeric_cast<float>(result.size());
  auto k_i         = float{1};
  while (!result.empty()) {
    status = it->second.wait_for(std::chrono::milliseconds{10});
    if (status == std::future_status::ready) {
      ++k_i;

      this->progress(boost::numeric_cast<int>((k_i / k_len) * 100));
      boost::format k_msg{"%s : %s\n"};
      k_msg % it->first.generic_string();
      try {
        it->second.get();
      } catch (const std::runtime_error &err) {
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