
#[[
寻找 pandoc
#]]


include(CMakePrintHelpers)

find_program(
        PANDOC
        pandoc.exe
        HINTS
        $ENV{LOCALAPPDATA}/pandoc
        DOC "pandoc exe path"
)
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Pandoc
        REQUIRED_VARS PANDOC
        REASON_FAILURE_MESSAGE "没有寻找到 pandoc.exe"
        )

add_executable(pandoc IMPORTED GLOBAL)

set_target_properties(
        pandoc
        PROPERTIES
        IMPORTED_LOCATION ${PANDOC}
)