cmake_minimum_required(VERSION 3.10)

project(test_bcryptgenrandom)

add_executable(
  ${PROJECT_NAME}
  test_BCryptGenRandom.cpp
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  libcompatwin
)
