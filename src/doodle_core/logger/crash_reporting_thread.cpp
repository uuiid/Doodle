//
// Created by td_main on 2023/8/29.
//

#include "crash_reporting_thread.h"

#include <boost/process.hpp>

#include "core/app_base.h"
#include <DbgHelp.h>
#include <Windows.h>
#include <fmt/chrono.h>
#include <wil/resource.h>
namespace doodle::detail {

namespace {
// LONG WINAPI UnhandledStaticInitException(LPEXCEPTION_POINTERS ExceptionInfo) {
//   g_ctx().get<crash_reporting_thread>().on_crash_during_static_init(ExceptionInfo);
//   return EXCEPTION_CONTINUE_SEARCH;
// }
LONG WINAPI DoodleUnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo) {
  //  ReportCrash(ExceptionInfo);
  //  GIsCriticalError = true;
  //  FPlatformMisc::RequestExit(true);
  // g_ctx().get<crash_reporting_thread>().stop();
  g_ctx().get<crash_reporting_thread>().on_crashed(ExceptionInfo);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  app_base::Get().stop_app(ExceptionInfo->ExceptionRecord->ExceptionCode);
  return EXCEPTION_EXECUTE_HANDLER;

  // return EXCEPTION_CONTINUE_SEARCH;  // Not really important, RequestExit() terminates the process just above.
}
}  // namespace
class crash_reporting_thread::impl {
 public:
  impl()  = default;
  ~impl() = default;
  DWORD thread_id_{};
  std::mutex mutex_;
  std::condition_variable condition_;
  LPEXCEPTION_POINTERS exception_info_;

  enum input_type : std::int32_t { crash, user_stop };

  std::atomic_int crash_count_{input_type::user_stop};

  std::shared_ptr<std::thread> thread_ptr_;
};
crash_reporting_thread::crash_reporting_thread() : impl_ptr_(std::make_unique<impl>()) {
  //  ::SetUnhandledExceptionFilter(UnhandledStaticInitException);
  impl_ptr_->thread_ptr_ = std::make_shared<std::thread>([this]() { this->run(); });
  ::SetThreadPriority(impl_ptr_->thread_ptr_->native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
  ::SetUnhandledExceptionFilter(DoodleUnhandledExceptionFilter);
}

void crash_reporting_thread::run() {
  std::unique_lock<std::mutex> l_lock{impl_ptr_->mutex_};
  impl_ptr_->condition_.wait(l_lock);
  if (impl_ptr_->crash_count_ == impl::input_type::crash) {
    handle_crash();
  }
}
void crash_reporting_thread::on_crash_during_static_init(LPEXCEPTION_POINTERS ExceptionInfo) {}
void crash_reporting_thread::on_crashed(LPEXCEPTION_POINTERS InExceptionInfo) {
  impl_ptr_->thread_id_      = ::GetCurrentThreadId();
  impl_ptr_->exception_info_ = InExceptionInfo;
  impl_ptr_->crash_count_    = impl::input_type::crash;
  impl_ptr_->condition_.notify_one();
}
void crash_reporting_thread::stop() {
  impl_ptr_->crash_count_ = impl::input_type::user_stop;
  impl_ptr_->condition_.notify_one();
}
crash_reporting_thread::~crash_reporting_thread() {
  if (impl_ptr_->thread_ptr_) {
    stop();
    impl_ptr_->thread_ptr_->join();
  }
}
void crash_reporting_thread::handle_crash() {
  FSys::path l_path{
      FSys::temp_directory_path() / "doodle" /
      fmt::format("Minidump_{:%Y_%m_%d_%H_%M_%S}.dmp", std::chrono::system_clock::now())
  };
  l_path = l_path.lexically_normal();
  l_path = l_path.make_preferred();
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  wil::unique_hfile l_file_handle{::CreateFileW(
      l_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
  )};
  if (!l_file_handle) {
    return;
  }
  MINIDUMP_EXCEPTION_INFORMATION l_exception_info{};
  l_exception_info.ThreadId          = impl_ptr_->thread_id_;
  l_exception_info.ExceptionPointers = impl_ptr_->exception_info_;
  l_exception_info.ClientPointers    = FALSE;
  //      MiniDumpNormal | MiniDumpWithPrivateReadWriteMemory | MiniDumpWithIndirectlyReferencedMemory |
  // MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo |
  // MiniDumpWithUnloadedModules
  MINIDUMP_TYPE l_f                  = MINIDUMP_TYPE(
      MiniDumpNormal | MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData |
      MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules
  );
  wil::unique_hmodule l_module{::GetModuleHandleW(L"dbghelp.dll")};
  auto l_func = reinterpret_cast<decltype(&::MiniDumpWriteDump)>(::GetProcAddress(l_module.get(), "MiniDumpWriteDump"));
  l_func(::GetCurrentProcess(), ::GetCurrentProcessId(), l_file_handle.get(), l_f, &l_exception_info, nullptr, nullptr);
}
}  // namespace doodle::detail