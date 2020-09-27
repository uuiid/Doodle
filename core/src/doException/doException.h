#pragma once

#include "core_global.h"
#include <stdexcept>

CORE_NAMESPACE_S

class doodle_notInsert : public std::runtime_error
{
public:
    virtual const char* what() const override;
};


CORE_DNAMESPACE_E