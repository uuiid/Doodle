#include "convert.h"

#include "Logger.h"

#include <QSqlError>

#include <QDebug>
#include <QTemporaryFile>

#include <thread>
#include <stdexcept>
PINYIN_NAMESPACE_S

QTemporaryFile convert::tmpDBFile;

convert::convert():isinitDB(false),re(),query(),dataBase(),sqlDBPath() {
  re = QSharedPointer<QRegularExpression>(new QRegularExpression());
}

convert::~convert() {
  dataBase.close();
}

std::string convert::toEn(const std::string &conStr) {
  if (!isinitDB)
    initDB();
  QRegularExpressionMatchIterator iter = re->globalMatch(QString::fromStdString(conStr));
  QString enStr = QString::fromStdString(conStr);
  while (iter.hasNext()) {
    QRegularExpressionMatch matchStr = iter.next();
    enStr = enStr.replace(matchStr.captured(), toEnOne(matchStr.captured()));
  }
  return enStr.toStdString();
}

void convert::initDB() {
  if (!createDB())
    throw std::runtime_error("not search DB");
  //使用线程id创建不一样的名字
  auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
  auto db_name = QString("%1%2").arg("sqlite_pinyin_db").arg(thread_id);

  if (QSqlDatabase::contains(db_name)) {
    dataBase = QSqlDatabase::database(db_name);
  } else {
    dataBase = QSqlDatabase::addDatabase("QSQLITE", db_name);
    dataBase.setDatabaseName(tmpDBFile.fileName());
  }
  if (!dataBase.open()) {
    DOODLE_LOG_WARN << dataBase.lastError().text();
    throw std::runtime_error(dataBase.lastError().text().toStdString());
  }
  dataBase.transaction();
  initQuery();
  initExp();
//  isinitDB = true;
}

void convert::initQuery() {
//  if (!query)
    query = QSharedPointer<QSqlQuery>(new QSqlQuery(dataBase));
}

void convert::initExp() {
  re->setPattern("[\u4e00-\u9fa5]");
}

bool convert::createDB() {
  if (tmpDBFile.exists())
    return true;
  if (tmpDBFile.open()) {
    QFile tmp(":/pinyin/pinyin.db");
    if (tmp.open(QIODevice::ReadOnly)) {
      tmpDBFile.write(tmp.readAll());
    }
    tmp.close();
  } else {
    return false;
  }
  tmpDBFile.close();
  return true;
}

QString convert::toEnOne(const QString &conStr) {
  QString sql = "SELECT en FROM pinyin WHERE znch='%1';";
  query->prepare(sql.arg(conStr));
  if (!query->exec()) {
    DOODLE_LOG_WARN << query->lastError();
    throw std::runtime_error(QString("not quert %1").arg(conStr).toStdString());
  }
  QString enstr("");
  if (query->next()) {
    enstr = query->value(0).toString();
  }
  return enstr.left(enstr.size() - 1);
}

DNAMESPACE_E
