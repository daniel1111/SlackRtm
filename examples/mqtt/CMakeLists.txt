add_executable(SlackMqtt  main.cpp)

target_link_libraries(SlackMqtt ${LIBRARY_NAME})
target_link_libraries(SlackMqtt -lmosquittopp)

install(TARGETS SlackMqtt
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
