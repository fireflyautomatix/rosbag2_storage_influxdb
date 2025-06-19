cmake_minimum_required(VERSION 3.8)
project(rosbag2_storage_influxdb)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies

set(dependencies
  pluginlib
  rosbag2_storage
  rosbag2_cpp
)

foreach(dependency ${dependencies})
  find_package(${dependency} REQUIRED)
endforeach()

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 1.10.2)
FetchContent_MakeAvailable(cpr)

add_library(${PROJECT_NAME} SHARED
  src/influxdb_storage.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC
  rosbag2_cpp::rosbag2_cpp
  rosbag2_storage::rosbag2_storage
  pluginlib::pluginlib
)
target_link_libraries(${PROJECT_NAME} PRIVATE
  cpr::cpr
)
target_compile_definitions(${PROJECT_NAME} PUBLIC RAPIDJSON_HAS_STDSTRING)

ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(${dependencies})

pluginlib_export_plugin_description_file(rosbag2_storage plugin_description.xml)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS cpr EXPORT ${PROJECT_NAME}Targets)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
