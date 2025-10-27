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

# Generate interfaces (actions and messages)
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/StartCapture.action"
  "msg/CameraStatus.msg"
)

# Camera node executable (receives start commands, publishes status and images)
add_executable(camera_node src/camera_node.cpp)
ament_target_dependencies(camera_node rclcpp rclcpp_action sensor_msgs std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target_camera ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(camera_node "${cpp_typesupport_target_camera}")

# Sync node executable (sends start capture commands to cameras)
add_executable(sync_node src/sync_node.cpp)
ament_target_dependencies(sync_node rclcpp rclcpp_action std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target_sync ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(sync_node "${cpp_typesupport_target_sync}")

# Maestro node executable (coordinates final synchronization)
add_executable(maestro_node src/maestro_node.cpp)
ament_target_dependencies(maestro_node rclcpp std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target_maestro ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(maestro_node "${cpp_typesupport_target_maestro}")

# Install executables
install(TARGETS
  camera_node
  sync_node
  maestro_node
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
