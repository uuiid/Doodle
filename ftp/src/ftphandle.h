#ifndef FTPHANDLE_H
#define FTPHANDLE_H

#include "ftp_global.h"
#include "ftpsession.h"
#include <QObject>
#include <QSharedPointer>
#include <QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
FTPSPACE_S

class FTP_EXPORT ftphandle:public QObject
{
    Q_OBJECT
public:
    ~ftphandle();
    ftphandle(const ftphandle&) =delete ;
    ftphandle& operator =(const ftphandle& s) =delete ;

    static ftphandle& getFTP();
    ftpSessionPtr session(const QString &host,
                        int prot,
                       const QString &name,
                       const QString &password);
private:
    ftphandle();

};

FTPSPACE_E

#endif // FTPHANDLE_H
