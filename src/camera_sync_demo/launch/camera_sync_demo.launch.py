from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Start the sync coordinator first
        Node(
            package='camera_sync_demo',
            executable='sync_coordinator',
            name='sync_coordinator',
            output='screen',
            emulate_tty=True,
        ),

        # Camera 1: Quick startup (500ms)
        Node(
            package='camera_sync_demo',
            executable='camera_node',
            name='camera_1',
            arguments=['camera_1', '192.168.1.101', '500'],
            output='screen',
            emulate_tty=True,
        ),

        # Camera 2: Medium startup (1500ms)
        Node(
            package='camera_sync_demo',
            executable='camera_node',
            name='camera_2',
            arguments=['camera_2', '192.168.1.102', '1500'],
            output='screen',
            emulate_tty=True,
        ),

        # Camera 3: Slow startup (3000ms)
        Node(
            package='camera_sync_demo',
            executable='camera_node',
            name='camera_3',
            arguments=['camera_3', '192.168.1.103', '3000'],
            output='screen',
            emulate_tty=True,
        ),

        # Camera 4: Very slow startup (5000ms)
        Node(
            package='camera_sync_demo',
            executable='camera_node',
            name='camera_4',
            arguments=['camera_4', '192.168.1.104', '5000'],
            output='screen',
            emulate_tty=True,
        ),
    ])
