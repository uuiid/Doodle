#include "episodes.h"

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/ShotModifySQLDate.h>

#include <loggerlib/Logger.h>
#include <boost/format.hpp>


DOODLE_NAMESPACE_S


DOODLE_INSRANCE_CPP(episodes);
episodes::episodes()
    : CoreData(),
      std::enable_shared_from_this<episodes>(),
      p_int_episodes(-1),
      p_prj(coreSet::getSet().getProject()),
      p_shot_modify() {
  p_instance.insert(this);
}

episodes::~episodes() {
  p_instance.erase(this);
}

episodesPtrList episodes::getAll() {
  return {};
}

bool episodes::setInfo(const std::string &value) {
  return true;
}

void episodes::setEpisdes(const int64_t &value) {
  p_int_episodes = value;
}

std::shared_ptr<ShotModifySQLDate> episodes::ShotModifySqlDate() const {
  return p_shot_modify;
}

episodesPtr episodes::find_by_eps(int64_t episodes_) {
  for (auto &&eps_ptr : p_instance) {
    if (eps_ptr->getEpisdes() == episodes_) {
      return eps_ptr->shared_from_this();
    }
  }
  return nullptr;
}

const std::unordered_set<episodes *> episodes::Instances() {
  return p_instance;
}

int64_t episodes::getEpisdes() const {
  return p_int_episodes;
}

dstring episodes::getEpisdes_str() const {
  boost::format str(DOODLE_EPFORMAT);
  str % p_int_episodes;
  return str.str();
}

DOODLE_NAMESPACE_E
