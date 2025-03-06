target_sources(
        doodle_core

        PUBLIC
        bcrypt.h

        PRIVATE
        bcrypt.cpp
        blowfish.cpp

)