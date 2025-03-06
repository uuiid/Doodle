target_sources(
        doodle_core

        PUBLIC
        bcrypt.h

        PRIVATE
        bcrypt.cpp
        blowfish.cpp

)

# copy from https://github.com/hilch/Bcrypt.cpp