target_sources(
        doodle_lib

        PUBLIC
        bcrypt.h

        PRIVATE
        bcrypt.cpp
        blowfish.cpp

)

# copy from https://github.com/hilch/Bcrypt.cpp