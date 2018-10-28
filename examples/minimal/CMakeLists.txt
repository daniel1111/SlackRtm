add_executable(SlackMinimal  main.cpp)

target_link_libraries(SlackMinimal ${LIBRARY_NAME})

install(TARGETS SlackMinimal
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
