//
// Created by TD on 2022/5/30.
//

#include "get_screenshot.h"
namespace doodle::win {
cv::Rect2f get_system_metrics_VIRTUALSCREEN() {
  //  int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  //  int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
  //  int width   = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  //  int height  = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return cv::Rect{GetSystemMetrics(SM_XVIRTUALSCREEN),
                  GetSystemMetrics(SM_YVIRTUALSCREEN),
                  GetSystemMetrics(SM_CXVIRTUALSCREEN),
                  GetSystemMetrics(SM_CYVIRTUALSCREEN)};
}

namespace {
BITMAPINFOHEADER createBitmapHeader(int width, int height) {
  BITMAPINFOHEADER bi;

  // create a bitmap
  bi.biSize          = sizeof(BITMAPINFOHEADER);
  bi.biWidth         = width;
  bi.biHeight        = -height;  // this is the line that makes it draw upside down or not
  bi.biPlanes        = 1;
  bi.biBitCount      = 32;
  bi.biCompression   = BI_RGB;
  bi.biSizeImage     = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed       = 0;
  bi.biClrImportant  = 0;

  return bi;
}
}  // namespace
cv::Mat get_screenshot() {
  auto hwnd = GetDesktopWindow();
  cv::Mat src{};
  // get handles to a device context (DC)
  HDC hwindowDC           = GetDC(hwnd);
  HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
  SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);
  // define scale, height and width
  auto k_rot = win::get_system_metrics_VIRTUALSCREEN();

  // create mat object
  src.create(k_rot.height, k_rot.width, CV_8UC4);

  // create a bitmap
  HBITMAP hbwindow    = CreateCompatibleBitmap(hwindowDC, k_rot.width, k_rot.height);
  BITMAPINFOHEADER bi = createBitmapHeader(k_rot.width, k_rot.height);

  // use the previously created device context with the bitmap
  SelectObject(hwindowCompatibleDC, hbwindow);

  // copy from the window device context to the bitmap device context
  StretchBlt(hwindowCompatibleDC, 0, 0, k_rot.width, k_rot.height,
             hwindowDC, k_rot.x, k_rot.y, k_rot.width, k_rot.height,
             SRCCOPY);  // change SRCCOPY to NOTSRCCOPY for wacky colors !
  GetDIBits(hwindowCompatibleDC, hbwindow,
            0, k_rot.height, src.data,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);  // copy from hwindowCompatibleDC to hbwindow

  // avoid memory leak
  DeleteObject(hbwindow);
  DeleteDC(hwindowCompatibleDC);
  ReleaseDC(hwnd, hwindowDC);
  return src;
}
#if 0
std::string get_font_data() {
  auto dc = ::GetDC(d3d_device::Get().handle_wnd);
  LOGFONTW logfont{};
  logfont.lfCharSet        = CHINESEBIG5_CHARSET;
  logfont.lfFaceName[0]    = '\0';
  logfont.lfPitchAndFamily = 0;

  LOGFONTW logfont_init{};

  std::string str{};
  ::EnumFontFamiliesExW(
      dc, &logfont,
      [](const LOGFONT* lpelfe,
         const TEXTMETRIC* lpntme,
         DWORD FontType,
         LPARAM lParam) {
        auto* font = reinterpret_cast<LOGFONTW*>(lParam);
        switch (FontType) {
          case DEVICE_FONTTYPE:
            break;

          case RASTER_FONTTYPE:
            break;

          case TRUETYPE_FONTTYPE:
            *font = *lpelfe;
            return 0;
            break;
          default:
            break;
        }
        return 1;
      },
      reinterpret_cast<LPARAM>(&logfont_init), 0);

  auto hfont = ::CreateFontIndirectW(&logfont_init);
  DOODLE_CHICK(hfont,doodle_error{ "无法获取字体数据"});
  ::SelectObject(dc, hfont);

  LPVOID ptr      = NULL;
  HGLOBAL hGlobal = NULL;

  auto l_size     = ::GetFontData(dc, 0, 0, nullptr, 0);
  DOODLE_CHICK(l_size != GDI_ERROR,doodle_error{ "无法获取字体数据"});
  std::unique_ptr<char[]> l_buff{new char[l_size]};

  hGlobal   = GlobalAlloc(GMEM_MOVEABLE, l_size);
  ptr       = GlobalLock(hGlobal);

  auto l_r_ = ::GetFontData(dc, 0, 0, ptr, l_size);
  DOODLE_CHICK(l_r_ != GDI_ERROR,doodle_error{ "无法获取字体数据"});
  IStream* fontStream = NULL;
  l_r_                = CreateStreamOnHGlobal(hGlobal, TRUE, &fontStream);
  ULONG _l_long{};
  fontStream->Read(l_buff.get(),l_size,&_l_long);

  std::string l_r{};
  std::copy_n(l_buff.get(), l_size, std::back_inserter(l_r));
  GlobalUnlock(hGlobal);
  return l_r;
}
#endif

}  // namespace doodle::win
