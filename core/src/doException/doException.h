#pragma once

#include "core_global.h"
#include <stdexcept>

CORE_NAMESPACE_S

class CORE_API doodle_notInsert : public std::runtime_error
{
public:
    doodle_notInsert(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_notFile : public std::runtime_error
{
public:
    doodle_notFile(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_CopyErr : public std::runtime_error
{
public:
    doodle_CopyErr(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};


//----------------------------------------------------------------
class CORE_API doodle_upload_error : public std::runtime_error
{
public:
    doodle_upload_error(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_InsertErrorInfo : public std::runtime_error
{
public:
    doodle_InsertErrorInfo(const std::string &err) : std::runtime_error(err){};
    virtual const char *what() const noexcept override;
};

CORE_NAMESPACE_E