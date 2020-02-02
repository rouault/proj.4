set(PROJ_DOWNLOAD_DATA_SRC apps/proj_download_data.cpp)

source_group("Source Files\\Bin" FILES ${PROJ_DOWNLOAD_DATA_SRC})

add_executable(bin_proj_download_data ${PROJ_DOWNLOAD_DATA_SRC})
set_target_properties(bin_proj_download_data
  PROPERTIES
  OUTPUT_NAME proj_download_data)
target_link_libraries(bin_proj_download_data ${PROJ_LIBRARIES})
target_compile_options(bin_proj_download_data PRIVATE ${PROJ_CXX_WARN_FLAGS})
install(TARGETS bin_proj_download_data
  RUNTIME DESTINATION ${BINDIR})

if(MSVC AND BUILD_LIBPROJ_SHARED)
  target_compile_definitions(bin_proj_download_data PRIVATE PROJ_MSVC_DLL_IMPORT=1)
endif()
