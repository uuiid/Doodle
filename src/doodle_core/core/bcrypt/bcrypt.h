#pragma once

#include <string>

namespace bcrypt {

    std::string generateHash(const std::string & password , unsigned rounds = 12 );

    bool validatePassword(const std::string & password, const std::string & hash);

}

