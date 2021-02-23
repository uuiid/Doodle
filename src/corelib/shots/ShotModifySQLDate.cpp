#include "ShotModifySQLDate.h"
#include <corelib/Exception/Exception.h>

#include <corelib/shots/episodes.h>
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>

#include <chrono>
// /*保护data里面的宏__我他妈的*/
// #ifdef min
// #undef min
// #endif
// #ifdef max
// #undef max
// #endif
// #include <date/date.h>
// /*保护data里面的宏__我他妈的*/

DOODLE_NAMESPACE_S

ShotModifySQLDate::ShotModifySQLDate(std::weak_ptr<episodes> &eps)
    : p_eps(std::move(eps)) {
}

void ShotModifySQLDate::selectModify() {

}
DOODLE_NAMESPACE_E