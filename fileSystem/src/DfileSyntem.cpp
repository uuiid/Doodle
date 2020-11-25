#include "DfileSyntem.h"
#include "ftpsession.h"

#include <QUrl>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <Logger.h>
#include <regex>

#include <boost/thread.hpp>
#include <boost/asio.hpp>

DSYSTEM_S
DfileSyntem::~DfileSyntem()
{
    curl_global_cleanup();
}

DfileSyntem &DfileSyntem::getFTP()
{
    static DfileSyntem install;
    return install;
}

ftpSessionPtr DfileSyntem::session(const std::string &host, int prot, const std::string &name, const std::string &password)
{
    ftpSessionPtr session(new ftpSession());
    session->setInfo(host,prot,name,password);
    return session;
}
bool DfileSyntem::copy(const dpath &sourePath, const dpath &trange_path) noexcept {
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());


  if (boost::filesystem::exists(trange_path)) return false;
  auto dregex = std::regex(sourePath.generic_string());
  DOODLE_LOG_INFO << sourePath.generic_string().c_str() << "-->" << trange_path.generic_string().c_str();
  for (auto &item : boost::filesystem::recursive_directory_iterator(sourePath)) {
    if (boost::filesystem::is_regular_file(item.path())) {
      dpath basic_string = std::regex_replace(item.path().generic_string(),
                                              dregex,
                                              trange_path.generic_string());
//      DOODLE_LOG_INFO << basic_string.generic_string().c_str();
      boost::asio::post(pool,[=](){
        boost::filesystem::create_directories(basic_string.parent_path());
        boost::filesystem::copy_file(item.path(), basic_string);
      });
//      try {
//
//      } catch (boost::filesystem::filesystem_error &error) {
//        DOODLE_LOG_WARN << error.what();
//      }
    }
  }
  pool.join();
  return true;
}
DfileSyntem::DfileSyntem()= default;

DSYSTEM_E
