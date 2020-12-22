# bin2cpp-config-version.cmake - checks version: major must match, minor must be less than or equal

set(PACKAGE_VERSION 2.3.0)

if("${PACKAGE_FIND_VERSION_MAJOR}" EQUAL "2")
    if ("${PACKAGE_FIND_VERSION_MINOR}" EQUAL "3")
        set(PACKAGE_VERSION_EXACT TRUE)
    elseif("${PACKAGE_FIND_VERSION_MINOR}" LESS "3")
        set(PACKAGE_VERSION_COMPATIBLE TRUE)
    else()
        set(PACKAGE_VERSION_UNSUITABLE TRUE)
    endif()
else()
    set(PACKAGE_VERSION_UNSUITABLE TRUE)
endif()
