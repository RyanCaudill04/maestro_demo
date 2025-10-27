cmake_minimum_required(VERSION 3.8)
project(camera_sync_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate action interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/CameraReady.action"
)

# Camera node executable
add_executable(camera_node src/camera_node.cpp)
ament_target_dependencies(camera_node rclcpp rclcpp_action sensor_msgs std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(camera_node "${cpp_typesupport_target}")

# Sync coordinator executable
add_executable(sync_coordinator src/sync_coordinator.cpp)
ament_target_dependencies(sync_coordinator rclcpp rclcpp_action std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target_sync ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(sync_coordinator "${cpp_typesupport_target_sync}")

# Install executables
install(TARGETS
  camera_node
  sync_coordinator
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
