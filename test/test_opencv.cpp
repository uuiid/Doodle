#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
wxImage wx_from_mat(cv::Mat &img) {
  cv::Mat im2;
  if (img.channels() == 1) {
    cv::cvtColor(img, im2, cv::COLOR_GRAY2RGB);
  } else if (img.channels() == 4) {
    cv::cvtColor(img, im2, cv::COLOR_BGRA2RGB);
  } else {
    cv::cvtColor(img, im2, cv::COLOR_BGR2RGB);
  }
  long imsize = im2.rows * im2.cols * im2.channels();
  wxImage wx(im2.cols, im2.rows, (unsigned char *)malloc(imsize), false);
  unsigned char *s = im2.data;
  unsigned char *d = wx.GetData();
  for (long i = 0; i < imsize; i++) {
    d[i] = s[i];
  }
  return wx;
}
cv::Mat mat_from_wx(wxImage &wx) {
  cv::Mat im2(cv::Size(wx.GetWidth(), wx.GetHeight()), CV_8UC3, wx.GetData());
  cvtColor(im2, im2, cv::COLOR_RGB2BGR);
  return im2;
}
//#include <QtGui/qimage.h>
//#include <QtCore/qdebug.h>
//#include <QtWidgets/qlabel.h>

//QImage cvMat2QImage(const cv::Mat &mat) {
//  // 8-bits unsigned, NO. OF CHANNELS = 1
//  if (mat.type() == CV_8UC1) {
//    QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
//    // Set the color table (used to translate colour indexes to qRgb values)
//    image.setColorCount(256);
//    for (int i = 0; i < 256; i++) {
//      image.setColor(i, qRgb(i, i, i));
//    }
//    // Copy input Mat
//    uchar *pSrc = mat.data;
//    for (int row = 0; row < mat.rows; row++) {
//      uchar *pDest = image.scanLine(row);
//      memcpy(pDest, pSrc, mat.cols);
//      pSrc += mat.step;
//    }
//    return image;
//  }
//  // 8-bits unsigned, NO. OF CHANNELS = 3
//  else if (mat.type() == CV_8UC3) {
//    // Copy input Mat
//    const uchar *pSrc = (const uchar *)mat.data;
//    // Create QImage with same dimensions as input Mat
//    QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
//    return image.rgbSwapped();
//  } else if (mat.type() == CV_8UC4) {
//    qDebug() << "CV_8UC4";
//    // Copy input Mat
//    const uchar *pSrc = (const uchar *)mat.data;
//    // Create QImage with same dimensions as input Mat
//    QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
//    return image.copy();
//  } else {
//    qDebug() << "ERROR: Mat could not be converted to QImage.";
//    return QImage();
//  }
//}
TEST(opencv, imageSequeTovideo) {
  const std::filesystem::path path{R"(D:\tmp\tmp)"};
  std::fstream file{};
  int i = 0;

  auto video = cv::VideoWriter("D:/test.mp4", cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), (double)25, cv::Size{1280, 720});
  cv::Mat image{};
  for (auto &&it_p : std::filesystem::directory_iterator(path)) {
    if (it_p.is_regular_file()) {
      image = cv::imread(it_p.path().string());
      video << image;
    }
  }
}
TEST(opencv, print_cv) {
  const std::filesystem::path path{R"(D:\image_test\BuJu.1001.jpg)"};
  auto k_mat = cv::imread(path.generic_string());
  auto k_gbr = k_mat.row(1).col(1);
  std::cout
      << "1,1 bgr-> " << k_gbr << "\n"
      << "info: " << k_mat.type() << "\n"
      << "channels: " << k_mat.channels() << "\n"
      << "elemSize: " << k_mat.elemSize() << "\n"
      << "elemSize1: " << k_mat.elemSize1() << "\n"
      << "step: " << k_mat.step << "\n"
      << "step1: " << k_mat.step1() << "\n"
      << std::endl;
}
