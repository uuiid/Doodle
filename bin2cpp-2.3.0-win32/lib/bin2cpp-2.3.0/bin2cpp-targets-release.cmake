#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "bin2cpp" for configuration "Release"
set_property(TARGET bin2cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(bin2cpp PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/bin2cpp.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS bin2cpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_bin2cpp "${_IMPORT_PREFIX}/bin/bin2cpp.exe" )

# Import target "testfilegenerator" for configuration "Release"
set_property(TARGET testfilegenerator APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(testfilegenerator PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/testfilegenerator.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS testfilegenerator )
list(APPEND _IMPORT_CHECK_FILES_FOR_testfilegenerator "${_IMPORT_PREFIX}/bin/testfilegenerator.exe" )

# Import target "bin2cpp_unittest" for configuration "Release"
set_property(TARGET bin2cpp_unittest APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(bin2cpp_unittest PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/bin2cpp_unittest.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS bin2cpp_unittest )
list(APPEND _IMPORT_CHECK_FILES_FOR_bin2cpp_unittest "${_IMPORT_PREFIX}/bin/bin2cpp_unittest.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
