add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44021)

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
  add_subdirectory( tables )
  add_subdirectory( queries )
  add_subdirectory( forms )
  add_subdirectory( reports )
  add_subdirectory( migration )
  add_subdirectory( importexport )

  if(KEXI_SCRIPTS_SUPPORT)
     add_subdirectory(scripting)
  endif()
endif ()
