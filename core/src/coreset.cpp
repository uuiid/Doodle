#include "coreset.h"
#include "coresql.h"

#include "src/convert.h"
#include "Logger.h"

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include "coreOrm/configure_sqlOrm.h"
#include "coreOrm/project_sqlOrm.h"
#include "coreOrm/synfile_sqlOrm.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <QStorageInfo>

#include <QRegularExpression>
#include <stdexcept>

CORE_NAMESPACE_S

const QString coreSet::settingFileName = "doodle_conf.json";

coreSet &coreSet::getCoreSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  doc = QDir::homePath() + "/Documents/doodle";
  getSetting();
  initdb();
  getServerSetting();
  getCacheDiskPath();
}

void coreSet::initdb() {
  coreSql &sql = coreSql::getCoreSql();
  sql.initDB(ipMysql);
}

void coreSet::writeDoodleLocalSet() {

  QJsonObject jsonobj;
  jsonobj.insert("user", user);
  jsonobj.insert("department", department);
  jsonobj.insert("synPath", synPath.absolutePath());
  jsonobj.insert("synEp", syneps);
  jsonobj.insert("projectname", QString::fromStdString(project.second));
  jsonobj.insert("FreeFileSync", freeFileSyn);
  QJsonDocument jsonDoc(jsonobj);
  QFile strFile(doc.absoluteFilePath(settingFileName));
  if (!strFile.open(QIODevice::WriteOnly))
    throw std::runtime_error("not open doodle_conf.json");
  strFile.write(jsonDoc.toJson(QJsonDocument::Compact));
  strFile.close();
}

coreSet::coreSet() {
  ipMysql = "192.168.10.213";
  user = "user";
  department = "VFX";
  syneps = 1;
  freeFileSyn = R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")";
  project = std::make_pair(3,"dubuxiaoyao3");
  synPath = QString("D:/ue_prj");
  synServer = "/03_Workflow/Assets";

  shotRoot = QString("/03_Workflow/Shots");
  assRoot = QString("/03_Workflow/Assets");
  prjectRoot = QString("W:/");

  cacheRoot = "C:/Doodle_cache";
  doc = QString("C:/Doodle_cache");
}

void coreSet::getSetting() {
  if (doc.exists(settingFileName)) {
    QFile strFile(doc.absoluteFilePath(settingFileName));
    if (!strFile.open(QIODevice::ReadOnly | QIODevice::Text))
      throw std::runtime_error("not open doodle_conf.json");

    QJsonParseError err;
    QJsonDocument jsondoc = QJsonDocument::fromJson(strFile.readAll(), &err);
    QJsonObject jsonObj = jsondoc.object();
    QJsonValue value;
    value = jsonObj.value("user");
    if (value != QJsonValue::Undefined)
      user = value.toString();
    value = jsonObj.value("department");
    if (value != QJsonValue::Undefined)
      department = value.toString();
    value = jsonObj.value("syn");
    if (value != QJsonValue::Undefined)
      synPath = value.toString();
    value = jsonObj.value("synEp");
    if (value != QJsonValue::Undefined)
      syneps = value.toInt();

    value = jsonObj.value("projectname");
    if (value != QJsonValue::Undefined)
      project.second = value.toString().toStdString();
    value = jsonObj.value("FreeFileSync");
    if (value != QJsonValue::Undefined)
      freeFileSyn = value.toString();
    strFile.close();
  } else {
    user = "none";
    department = "none";
    synPath = QString("D:/ue_prj");
    syneps = 1;
    projectname = "dubuxiaoyao3";
    freeFileSyn = QString(R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")");
    writeDoodleLocalSet();
  }
}

QString coreSet::toIpPath(const QString &path) {
  static QRegularExpression exp("^[A-Z]:");
//  DOODLE_LOG_INFO << exp.match(path);
  if (exp.match(path).hasMatch()) {
    return path.right(path.size() - 2);
  }
  return path;
}

QString coreSet::getDepartment() const {
  return department;
}

void coreSet::setDepartment(const QString &value) {
  department = value;
}

QString coreSet::getUser() const {
  return user;
}

QString coreSet::getUser_en() const {
  dopinyin::convert con;
  return QString::fromStdString(con.toEn(user.toStdString())).toLower();
}

void coreSet::setUser(const QString &value) {
  user = value;
}

QString coreSet::getIpMysql() const {
  return ipMysql;
}

void coreSet::setIpMysql(const QString &value) {
  ipMysql = value;
}

QString coreSet::getIpFtp() const {
  return ipFTP;
}

void coreSet::setIpFtp(const QString &value) {
  ipFTP = value;
}

QDir coreSet::getDoc() const {
  return doc;
}

QDir coreSet::getCacheRoot() const {
  return cacheRoot;
}

QDir coreSet::getPrjectRoot() const {
  return prjectRoot;
}

void coreSet::setPrjectRoot(const QDir &value) {
  prjectRoot = value;
}

QDir coreSet::getAssRoot() const {
  return assRoot;
}

void coreSet::setAssRoot(const QDir &value) {
  assRoot = value;
}

QDir coreSet::getShotRoot() const {
  return shotRoot;
}

void coreSet::setShotRoot(const QDir &value) {
  shotRoot = value;
}

QString coreSet::getProjectname() {
  return QString::fromStdString(project.second);
}

void coreSet::setProjectname(const QString &value) {
  setProjectname(value.toStdString());
}

QString coreSet::getFreeFileSyn() const {
  return freeFileSyn;
}

void coreSet::setFreeFileSyn(const QString &value) {
  freeFileSyn = value;
}

int coreSet::getSyneps() const {
  return syneps;
}

void coreSet::setSyneps(int value) {
  syneps = value;
}

void coreSet::getServerSetting() {
  //获得项目个数
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Project prjTab;
  for (auto &&raw: db->run(sqlpp::select(sqlpp::all_of(prjTab)).from(prjTab).unconditionally())) {
    prjMap.insert(std::make_pair<int, std::string>((int) raw.id, (std::string) raw.name));
  }
  setProjectname(project.second);

  doodle::Configure tab;
  std::map<std::string, std::string> map;
  for (auto &&raw: db->run(sqlpp::select(tab.name, tab.value)
                               .from(tab)
                               .where(tab.projectId == project.first))) {
    map.insert(std::make_pair<std::string, std::string>(raw.name, raw.value));
  }
  shotRoot = QString::fromStdString(map["shotRoot"]);
  assRoot = QString::fromStdString(map["assetsRoot"]);
  synServer = QString::fromStdString(map["synSever"]);
  prjectRoot = QString::fromStdString(map["project"]);

  if (map.find("IP_FTP") != map.end())
    ipFTP = QString::fromStdString(map["IP_FTP"]);
  else
    ipFTP = ipMysql;

//  QString sql = "SELECT DISTINCT name,value FROM %1.`configure`";
//  sql = sql.arg(projectname);
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  query->exec(sql);
//  mapStringPtr mapSet;
//  for (int i = 0; i < 5; i++) {
//    query->next();
//    if (!query->value(0).isNull() && !query->value(1).isNull())
//      mapSet.insert(query->value(0).toString(), query->value(1).toString());
//  }
//  shotRoot = mapSet.value("shotRoot");
//  assRoot = mapSet.value("assetsRoot");
//  synServer = mapSet.value("synSever");
//  prjectRoot = mapSet.value("project");
//  if (!mapSet.value("IP_FTP").isNull())
//    ipFTP = mapSet["IP_FTP"];
//  else
//    ipFTP = ipMysql;
}

synPathListPtr coreSet::getSynDir() {
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Synfile table;
  for (auto &&row:db->run(sqlpp::select(table.path)
                              .where(table.projectId == project.first))) {
  DOODLE_LOG_INFO << QString::fromStdString(row.path);
  }

  synPathListPtr list;
//  QString sql = "SELECT DISTINCT value3, value4 FROM %1.`configure` "
//                "WHERE name='synpath' AND value='%2' AND value2 ='%3'";
//  sql = sql.arg(projectname).arg(department).arg(syneps, 3, 10, QLatin1Char('0'));
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  query->exec(sql);
//  while (query->next()) {
//    synPath_struct synpath_;
//    synpath_.local = QString("%1%2/%3")
//        .arg(synPath.absolutePath())
//        .arg(projectname)
//        .arg(query->value(1).toString());
//    query->next();
//    synpath_.server = QString("%2/%3/%4")
//        .arg(toIpPath(synServer.absolutePath()))
//        .arg(department)
//        .arg(query->value(1).toString());
//    list.append(synpath_);
//  }
  return list;
}

void coreSet::getCacheDiskPath() {
  for (QStorageInfo &x : QStorageInfo::mountedVolumes()) {
    if (x.isValid() && x.isReady()) {
      if (!x.isReadOnly()) {
        if (((double) x.bytesAvailable() / (double) x.bytesTotal() > 0.5f) && (!x.isRoot())) {
          cacheRoot = x.rootPath() + "Doodle_cache";
          break;
        }
      }
    }
  }
}
QStringList coreSet::getAllPrjName() const {
  QStringList list;
  for (auto &&prj :prjMap) {
    list.push_back(QString::fromStdString(prj.second));
  }
  return list;
}
const QFileInfo &coreSet::getSynPathLocale() const {
  return synPath;
}
void coreSet::setSynPathLocale(const QFileInfo &syn_path) {
  synPath = syn_path;
}
std::pair<int, std::string> coreSet::projectName() const {
  return project;
}
void coreSet::setProjectname(const std::string &value) {
  auto result = std::find_if(prjMap.begin(), prjMap.end(),
                             [=](const auto &mo) { return mo.second == value; });
  if (result != prjMap.end())
    project = *result;
}
CORE_NAMESPACE_E
