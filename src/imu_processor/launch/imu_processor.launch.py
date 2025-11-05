from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Start the IMU publisher node
        Node(
            package='imu_processor',
            executable='imu_publisher_node',
            name='imu_publisher_node',
            output='screen',
            emulate_tty=True,
        ),
    ])
