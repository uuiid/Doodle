# project(Doodle_Maya_plug
#         VERSION 2.3.1.9
#         LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/Autodesk/Maya2020")
set(CMAKE_PREFIX_PATH "F:/Qt/5.12.9/msvc2017_64")
message(${CMAKE_PREFIX_PATH})
find_package(Qt5 CONFIG COMPONENTS Core Widgets Gui LinguistTools  REQUIRED)
# set(AUTOMOC_EXECUTABLE "C:/Program Files/Autodesk/Maya2020/bin/moc.exe")

# helper macro to set up a moc rule
function(Maya_qt5_create_moc_command infile outfile moc_flags moc_options moc_target moc_depends)
    _qt5_warn_deprecated("Maya_qt5_create_moc_command")

    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    get_filename_component(_moc_outfile_name "${outfile}" NAME)
    get_filename_component(_moc_outfile_dir "${outfile}" PATH)
    if(_moc_outfile_dir)
        set(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    endif()
    set (_moc_parameters_file ${outfile}_parameters)
    set (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    string (REPLACE ";" "\n" _moc_parameters "${_moc_parameters}")

    if(moc_target)
        set(_moc_parameters_file ${_moc_parameters_file}$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>)
        set(targetincludes "$<TARGET_PROPERTY:${moc_target},INCLUDE_DIRECTORIES>")
        set(targetdefines "$<TARGET_PROPERTY:${moc_target},COMPILE_DEFINITIONS>")

        set(targetincludes "$<$<BOOL:${targetincludes}>:-I$<JOIN:${targetincludes},\n-I>\n>")
        set(targetdefines "$<$<BOOL:${targetdefines}>:-D$<JOIN:${targetdefines},\n-D>\n>")

        file (GENERATE
            OUTPUT ${_moc_parameters_file}
            CONTENT "${targetdefines}${targetincludes}${_moc_parameters}\n"
        )

        set(targetincludes)
        set(targetdefines)
    else()
        file(WRITE ${_moc_parameters_file} "${_moc_parameters}\n")
    endif()

    set(_moc_extra_parameters_file @${_moc_parameters_file})
    add_custom_command(OUTPUT ${outfile}
                       COMMAND "C:/Program Files/Autodesk/Maya2020/bin/moc.exe" ${_moc_extra_parameters_file}
                       DEPENDS ${infile} ${moc_depends}
                       ${_moc_working_dir}
                       VERBATIM)
    set_source_files_properties(${infile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOUIC ON)
endfunction()

function(Maya_qt5_wrap_cpp outfiles)
    set(_QT5_INTERNAL_SCOPE ON)

    # get include dirs
    qt5_get_moc_flags(moc_flags)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs OPTIONS DEPENDS)

    cmake_parse_arguments(_WRAP_CPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(moc_files ${_WRAP_CPP_UNPARSED_ARGUMENTS})
    set(moc_options ${_WRAP_CPP_OPTIONS})
    set(moc_target ${_WRAP_CPP_TARGET})
    set(moc_depends ${_WRAP_CPP_DEPENDS})

    foreach(it ${moc_files})
        get_filename_component(it ${it} ABSOLUTE)
        qt5_make_output_file(${it} moc_ cpp outfile)
        Maya_qt5_create_moc_command(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

add_subdirectory(lib)
add_subdirectory(MayaPlug)
