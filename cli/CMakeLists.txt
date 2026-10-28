add_executable(nanoid_cli main.cpp)
target_link_libraries(nanoid_cli PRIVATE nanoid::nanoid)
set_target_properties(nanoid_cli PROPERTIES OUTPUT_NAME nanoid)

if (NANOID_INSTALL)
    install(TARGETS nanoid_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
