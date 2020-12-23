#include "Logger.h"
#include "LoggerTemplate.h"

//windows头
#include <windows.h>
#include <Shlobj.h>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/locale.hpp>

#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/filesystem.hpp>
namespace Logger {
using file_sink = boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>;
using sink_t    = boost::log::sinks::synchronous_sink<boost::log::sinks::debug_output_backend>;
// New macro that includes severity, filename and line number
#define LOG_ADD_FILE boost::log::attribute_cast<boost::log::attributes::mutable_constant<std::string>>(::boost::log::trivial::logger::get().get_attributes()["File"]).set(__FILE__)
#define LOG_ADD_LINE boost::log::attribute_cast<boost::log::attributes::mutable_constant<int>>(::boost::log::trivial::logger::get().get_attributes()["Line"]).set(__LINE__)

// static QString gLogDir;
// static int gLogMaxCount;

// static void outputMessage(QtMsgType type, const QMessageLogContext &context,
//                           const QString &msg);
// static void outputMessageAsync(QtMsgType type,
//                                const QMessageLogContext &context,
//                                const QString &msg);

static void
boostLoggerInitAsyn(const std::string &logPath,
                    std::size_t logMaxSize);

void boostLoggerInitAsyn(const std::string &logPath,
                         std::size_t logMaxSize) {
  char tmp[256];
  boost::filesystem::path appdata{};
  auto appdata_res = SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, tmp);
  if (appdata_res != S_OK)
    appdata = boost::dll::program_location().parent_path();
  else
    appdata = tmp;

    // boost::log::add_file_log(
    //     boost::log::keywords::file_name = appdata / "doodle" / "log" / "doodle_%Y_%m_%d_%H_%M_%S.%5N.html",
    //     boost::log::keywords::rotation_size = 10 * 1024 * 1024,
    //     boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_interval(boost::posix_time::hours(1)),
    //     boost::log::keywords::format =
    //         boost::log::expressions::stream
    //         << boost::log::trivial::severity << "\t"
    //         << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S") << "\t"
    //         << boost::log::expressions::smessage
    //         << "\r\n");
    // boost::log::core::get()->set_filter(
    //     boost::log::trivial::severity >= boost::log::trivial::debug);

#ifdef NDEBUG
  boost::shared_ptr<file_sink> sink{new file_sink{
      boost::log::keywords::target              = appdata / boost::filesystem::basename(boost::dll::program_location()) / "log",
      boost::log::keywords::file_name           = appdata / boost::filesystem::basename(boost::dll::program_location()) / "log" / logPath,
      boost::log::keywords::rotation_size       = 10 * 1024 * 1024,
      boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_interval(boost::posix_time::hours(1)),
      boost::log::keywords::max_size            = logMaxSize,
      boost::log::keywords::max_files           = 1024}};
  // clang-format off
  sink->set_formatter(
      boost::log::expressions::stream
      << "<div class=\"" << boost::log::trivial::severity << "\">"
      << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp","%Y-%m-%d %H:%M:%S")
      << boost::log::expressions::smessage
      << "</div>\n"
      );//, "%H:%M:%S.%f"
  // clang-format on
  sink->locked_backend()->set_open_handler([=](boost::log::sinks::text_file_backend::stream_type &file) {
    file << logTemplate;
  });
  sink->locked_backend()->set_close_handler([=](boost::log::sinks::text_file_backend::stream_type &file) {
    file << " \n<body>";
  });
  sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);

  sink->locked_backend()->auto_flush(true);
  boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
  boost::log::core::get()->add_sink(sink);

  sink->imbue(boost::locale::generator()("zh_CN.UTF-8"));
#else
  //debug 记录器
  boost::shared_ptr<sink_t> sink_t(new sink_t());
  sink->set_filter(boost::log::expressions::is_debugger_present() && (boost::log::trivial::severity >= boost::log::trivial::debug));
  boost::log::core::get()->add_sink(sink_t);
  sink_t->imbue(boost::locale::generator()("zh_CN.UTF-8"));
#endif  //NDEBUG
  BOOST_LOG_TRIVIAL(debug)
      << "log日志文件初始化成功";
  boost::log::core::get()->flush();
}

void doodle_initLog(const std::string &logPath, std::size_t logMaxSize, bool async) {
  boostLoggerInitAsyn(logPath, logMaxSize);
  // if (async) {
  //   qInstallMessageHandler(outputMessageAsync);
  // } else {
  //   qInstallMessageHandler(outputMessage);
  // }
  // QStringList gListDir =
  //     QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
  // if (!gListDir.isEmpty()) {
  //   gLogDir = gListDir[0] + "/" + logPath;
  // }
  // gLogMaxCount = logMaxCount;
  // QDir dir(gLogDir);
  // if (!dir.exists()) {
  //   dir.mkpath(dir.absolutePath());
  // }
  // QStringList infoList = dir.entryList(QDir::Files, QDir::Name);
  // while (infoList.size() > gLogMaxCount) {
  //   dir.remove(infoList.first());
  //   infoList.removeFirst();
  // }
}

// static void outputMessageAsync(QtMsgType type,
//                                const QMessageLogContext &context,
//                                const QString &msg) {
//   static const QString messageTemp = QString("<div class=\"%1\">%2</div>\r\n");
//   static const char typeList[] = {'d', 'w', 'c', 'f', 'i'};
//   static QMutex mutex;
//   static QFile file;
//   static QTextStream textStream;
//   static uint count = 0;
//   static const uint maxCount = 512;
//   Q_UNUSED(context);
//   QDateTime dt = QDateTime::currentDateTime();
//   //每小时一个文件
//   QString fileNameDt = dt.toString("yyyy-MM-dd-hh");

//   //每分钟一个文件
//   // QString fileNameDt = dt.toString("yyyy-MM-dd_hh_mm");

//   QString contentDt = dt.toString("yyyy-MM-dd hh:mm:ss");
//   QString message = QString("%1 %2").arg(contentDt).arg(msg);
//   QString htmlMessage =
//       messageTemp.arg(typeList[static_cast<int>(type)]).arg(message);
//   QString newfileName = QString("%1/%2_log.html").arg(gLogDir).arg(fileNameDt);
//   mutex.lock();
//   if (file.fileName() != newfileName) {
//     if (file.isOpen()) {
//       file.close();
//     }
//     file.setFileName(newfileName);
//     bool exist = file.exists();
//     file.open(QIODevice::WriteOnly | QIODevice::Append);
//     textStream.setDevice(&file);
//     textStream.setCodec("UTF-8");
//     if (!exist) {
//       textStream << QString::fromStdString(logTemplate) << "\r\n";
//     }
//   }
//   textStream << htmlMessage;
//   textStream.flush();
//   count += static_cast<uint>(htmlMessage.length());
//   if (count >= maxCount) {
//     file.close();
//     file.open(QIODevice::WriteOnly | QIODevice::Append);
//   }
//   mutex.unlock();
// #ifdef Q_OS_WIN
//   // ::OutputDebugString(reinterpret_cast<LPCSTR>(message.toStdWString().data()));
//   // ::OutputDebugString(reinterpret_cast<LPCSTR>(L"\r\n"));
// #else
//   fprintf(stderr, message.toStdString().data());
// #endif
// }
// static void outputMessage(QtMsgType type, const QMessageLogContext &context,
//                           const QString &msg) {
//   static const QString messageTemp = QString("<div class=\"%1\">%2</div>\r\n");
//   static const char typeList[] = {'d', 'w', 'c', 'f', 'i'};
//   static QMutex mutex;

//   Q_UNUSED(context);
//   QDateTime dt = QDateTime::currentDateTime();

//   //每小时一个文件
//   QString fileNameDt = dt.toString("yyyy-MM-dd_hh");

//   //每分钟一个文件
//   // QString fileNameDt = dt.toString("yyyy-MM-dd_hh_mm");

//   QString contentDt = dt.toString("yyyy-MM-dd hh:mm:ss");
//   QString message = QString("%1 %2").arg(contentDt).arg(msg);
//   QString htmlMessage =
//       messageTemp.arg(typeList[static_cast<int>(type)]).arg(message);
//   QFile file(QString("%1/%2_log.html").arg(gLogDir).arg(fileNameDt));
//   mutex.lock();

//   bool exist = file.exists();
//   file.open(QIODevice::WriteOnly | QIODevice::Append);
//   QTextStream textStream(&file);
//   textStream.setCodec("UTF-8");
//   if (!exist) {
//     textStream << QString::fromStdString(logTemplate) << "\r\n";
//   }
//   textStream << htmlMessage;
//   file.close();
//   mutex.unlock();
// #ifdef Q_OS_WIN
//   ::OutputDebugString(reinterpret_cast<LPCSTR>(message.toStdWString().data()));
//   ::OutputDebugString(reinterpret_cast<LPCSTR>(L"\r\n"));
// #else
//   fprintf(stderr, message.toStdString().data());
// #endif
// }
}  // namespace Logger
