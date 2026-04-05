find_package(GTest QUIET)

if(NOT GTest_FOUND)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

include_directories(${${PROJECT_NAME}_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}.test
  AllTests.cpp
  tests.cpp)

if(TARGET GTest::gtest AND TARGET GTest::gmock)
  target_link_libraries(${PROJECT_NAME}.test
    ${PROJECT_NAME}
    GTest::gtest
    GTest::gmock
  )
else()
  target_link_libraries(${PROJECT_NAME}.test
    ${PROJECT_NAME}
    gtest
    gmock
  )
endif()

install(TARGETS ${PROJECT_NAME}.test
        DESTINATION "${tool_dest}")
