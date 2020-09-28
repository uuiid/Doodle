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

ftpSessionPtr ftphandle::session(const QString &host, int prot, const QString &name, const QString &password)
{
    ftpSessionPtr session(new ftpSession());
    session->setInfo(host,prot,name,password);
    return session;
}

ftphandle::ftphandle()
{
}
FTPSPACE_E
