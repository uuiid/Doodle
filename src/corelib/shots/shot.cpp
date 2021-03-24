#include "shot.h"
#include <corelib/Exception/Exception.h>
#include <corelib/shots/ShotModifySQLDate.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>

#include <loggerlib/Logger.h>
#include <boost/format.hpp>

#include <magic_enum.hpp>

#include <memory>

DOODLE_NAMESPACE_S

DOODLE_INSRANCE_CPP(shot);
const std::vector<std::string> shot::e_shotAB_list = {"B", "C", "D", "E",
                                                      "F", "G", "H"};

shot::shot()
    : CoreData(),
      std::enable_shared_from_this<shot>(),
      p_qint_shot_(-1),
      p_qenm_shotab(e_shotAB::_),
      p_eps_id(-1),
      p_ptr_eps(),
      p_inDeadline(false) {
  p_instance.insert(this);
}

shot::~shot() {
  p_instance.erase(this);
}

bool shot::setInfo(const std::string &value) {
  return true;
}

shotPtrList shot::getAll(const episodesPtr &EP_) {
  return {};
}

void shot::setEpisodes(const episodesPtr &value) {
  if (!value) return;
  p_ptr_eps = value;
}

episodesPtr shot::getEpisodes() {
  if (p_ptr_eps)
    return p_ptr_eps;
  else
    throw nullptr_error("");
}

void shot::setShot(const int64_t &sh, const e_shotAB &ab) {
  p_qint_shot_  = sh;
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
    case e_shotAB::_:
      str = "";
      break;
    default:
      std::string tmpstr(magic_enum::enum_name(p_qenm_shotab));
      str = tmpstr;
      break;
  }
  return str;
}

bool shot::inDeadline() const {
  return p_inDeadline;
}

const std::unordered_set<shot *> shot::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
