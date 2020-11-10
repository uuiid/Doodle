#include "ftphandle.h"
#include "ftpsession.h"

#include <QUrl>
#include <curl/curl.h>
#include <stdexcept>

FTPSPACE_S
ftphandle::~ftphandle()
{
    curl_global_cleanup();
}

ftphandle &ftphandle::getFTP()
{
    static ftphandle install;
    return install;
}

ftpSessionPtr ftphandle::session(const std::string &host, int prot, const std::string &name, const std::string &password)
{
    ftpSessionPtr session(new ftpSession());
    session->setInfo(host,prot,name,password);
    return session;
}

ftphandle::ftphandle()= default;
FTPSPACE_E
