#include "shot.h"

#include "episodes.h"
#include "coresql.h"

#include "coreOrm/shots_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <magic_enum.hpp>

#include <memory>
#include <boost/format.hpp>
#include <Logger.h>

#include <src/coreDataManager.h>
CORE_NAMESPACE_S

const std::vector<std::string> shot::e_shotAB_list = {"B", "C", "D", "E", "F", "G", "H"};

shot::shot()
    : coresqldata(),
      std::enable_shared_from_this<shot>(),
      p_qint_shot_(-1),
      p_qenm_shotab(e_shotAB::_),
      p_eps_id(-1),
      p_ptr_eps() {
}

void shot::select(const qint64 &ID_) {
  doodle::Shots table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row: db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.id == ID_)
  )) {
    idP = row.id;
    p_qint_shot_ = row.shot;
    setShotAb((dstring) row.shotab);
    p_eps_id = row.episodesId;
  }

}

void shot::insert() {
  if (idP > 0) return;

  doodle::Shots table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::insert_into(table).columns(table.episodesId,
                                                   table.shot,
                                                   table.shotab);

  install.values.add(table.episodesId = p_eps_id,
                     table.shot = p_qint_shot_,
                     table.shotab = sqlpp::value_or_null<dstring>(getShotAb_str()));
  idP = db->insert(install);
  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入镜头" << p_qint_shot_;
    throw std::runtime_error("not install shots");
  }
  coreDataManager::get().setShotL(shared_from_this());
}

void shot::updateSQL() {

}

void shot::deleteSQL() {
  doodle::Shots table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}

shotPtrList shot::getAll(const episodesPtr &EP_) {
  shotPtrList list{};
  doodle::Shots table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row: db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.episodesId == EP_->getIdP())
          .order_by(table.shot.asc(), table.shotab.asc())
  )) {
    auto item = std::make_shared<shot>();
    item->idP = row.id;
    item->p_qint_shot_ = row.shot;
    item->setShotAb(row.shotab);
    item->setEpisodes(EP_);
    list.push_back(item);
  }
  coreDataManager::get().setShotL(list);
  return list;
}

void shot::setEpisodes(const episodesPtr &value) {
  if (!value)
    return;
  p_eps_id = value->getIdP();
  p_ptr_eps = value;
}

episodesPtr shot::getEpisodes() {
  if (p_ptr_eps)
    return p_ptr_eps;
  else {
    p_ptr_eps = std::make_shared<episodes>();
    p_ptr_eps->select(p_eps_id);
    return p_ptr_eps;
  }
}

void shot::setShot(const int64_t &sh, const e_shotAB &ab) {
  p_qint_shot_ = sh;
  p_qenm_shotab = ab;
}

void shot::setShot(const int64_t &sh, const dstring &ab) {
  p_qint_shot_ = sh;
  setShotAb(ab);
}

void shot::setShotAb(const dstring &ab) {
  if (ab.empty())
    p_qenm_shotab = e_shotAB::_;
  else {
    auto enum_ab = magic_enum::enum_cast<e_shotAB>(ab);
    if (enum_ab.has_value()) {
      p_qenm_shotab = enum_ab.value();
    }
  }
}

dstring shot::getShotAndAb_str() const {
  boost::format str("%1%%2%");
  str % getShot_str() % getShotAb_str();
  return str.str();
}

dstring shot::getShot_str() const {
  boost::format str(DOODLE_SHFORMAT);
  str % p_qint_shot_;
  return str.str();
}

dstring shot::getShotAb_str() const {
  dstring str;
  switch (p_qenm_shotab) {
    case e_shotAB::_:str = "";
      break;
    default:std::string tmpstr(magic_enum::enum_name(p_qenm_shotab));
      str = tmpstr;
      break;
  }
  return str;
}
CORE_NAMESPACE_E
