FROM ros:humble

# Install additional ROS2 packages
RUN apt-get update && apt-get install -y \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-camera-info-manager \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Set up workspace
WORKDIR /ros2_ws

# Source ROS2 on container start
RUN echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
RUN echo "if [ -f /ros2_ws/install/setup.bash ]; then source /ros2_ws/install/setup.bash; fi" >> ~/.bashrc

CMD ["/bin/bash"]
