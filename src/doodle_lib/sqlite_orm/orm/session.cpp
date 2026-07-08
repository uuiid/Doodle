#include <doodle_lib/sqlite_orm/orm/session.h>

#include "storage.h"


namespace doodle::orm {

session::session(storage& s) : connection_guard_(s), s_(&s) {}

session::~session() = default;

}  // namespace doodle::orm