file(GLOB header_files ./../../../include/darcel/reactors_tests/*.hpp)
file(GLOB source_files ./../../../source/reactors_tests/*.cpp)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

add_executable(reactors_tests ${source_files})
set_source_files_properties(${header_files} PROPERTIES HEADER_FILE_ONLY TRUE)
add_custom_command(TARGET reactors_tests POST_BUILD COMMAND reactors_tests)
install(TARGETS reactors_tests CONFIGURATIONS Debug
  DESTINATION $ENV{TEST_INSTALL_DIRECTORY}/debug)
install(TARGETS reactors_tests CONFIGURATIONS Release RelWithDebInfo
  DESTINATION $ENV{TEST_INSTALL_DIRECTORY}/release)
