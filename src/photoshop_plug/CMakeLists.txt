find_package(Photoshop ${Photoshop_Version})


if(TARGET Photoshop::Photoshop)
  
  add_library(
    doodle_ps_plug
    MODULE
  )
  add_subdirectory(main)

endif()
