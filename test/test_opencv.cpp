#include <gtest/gtest.h>

#include <opencv2/video.hpp>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <fstream>
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
TEST(opencv, videoToQimage) {
  //  const std::filesystem::path path{R"(test.mp4)"};
  //  std::fstream file{};
  //  int i = 0;
  //
  //  auto video = cv::VideoCapture("D:/test2.mp4");
  //
  //  auto label = QLabel();
  //  for (size_t i = 0; i < 10; ++i) {
  //    QImage q_image{};
  //    cv::Mat image;
  //    cv::Mat image2;
  //    if (!video.read(image))
  //      continue;
  //    cv::cvtColor(image, image2, cv::COLOR_BGR2RGB);
  //
  //    // cv::imshow("test", image);
  //    // q_image = cvMat2QImage(image);
  //    q_image = QImage{
  //        (const unsigned char *)image2.data,
  //        image2.cols,
  //        image2.rows,
  //        (int)image2.step,
  //        QImage::Format::Format_RGB888};
  //    auto test_image = q_image.copy();
  //    test_image.save(QString{"D:/tmp/qt/tset_"} + QString::number(i) + ".jpg");
  //    // label.setPixmap(QPixmap::fromImage(test_image));
  //    // label.show();
  //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  //  }
}