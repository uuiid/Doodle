#include "doException.h"

CORE_NAMESPACE_S

const char *doodle_notInsert::what() const
{
    return "Unable to insert db";
}

CORE_DNAMESPACE_E