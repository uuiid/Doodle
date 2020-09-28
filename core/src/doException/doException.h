#pragma once

#include "core_global.h"
#include <stdexcept>

CORE_NAMESPACE_S

class doodle_notInsert : public std::runtime_error
{
public:
    doodle_notInsert(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

class doodle_notFile : public std::runtime_error
{
public:
    doodle_notFile(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

class doodle_CopyErr : public std::runtime_error
{
public:
    doodle_CopyErr(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

class doodle_upload_error : public std::runtime_error
{
public:
    doodle_upload_error(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

CORE_DNAMESPACE_E