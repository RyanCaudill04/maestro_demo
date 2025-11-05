#!/bin/bash

# Script to launch imu_processor package
# This script handles process management and graceful shutdown

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Package name
PACKAGE_NAME="imu_processor"

# Store PID of the ROS launch process
LAUNCH_PID=""

# Cleanup function - called on script exit
cleanup() {
    if [ -n "$LAUNCH_PID" ]; then
        echo -e "\n${YELLOW}[${PACKAGE_NAME}] Shutting down...${NC}"

        # Send SIGINT (Ctrl+C) to the process group
        kill -SIGINT -$LAUNCH_PID 2>/dev/null || true

        # Wait a bit for graceful shutdown
        sleep 2

        # Force kill if still running
        if kill -0 $LAUNCH_PID 2>/dev/null; then
            echo -e "${YELLOW}[${PACKAGE_NAME}] Force killing...${NC}"
            kill -SIGKILL -$LAUNCH_PID 2>/dev/null || true
        fi

        echo -e "${GREEN}[${PACKAGE_NAME}] Stopped${NC}"
    fi
}

# Register cleanup function for various signals
trap cleanup EXIT SIGINT SIGTERM

# Check if we're in the right directory
if [ ! -f "install/setup.bash" ]; then
    echo -e "${RED}Error: install/setup.bash not found${NC}"
    echo "Please run 'colcon build' first or run this script from the workspace root"
    exit 1
fi

# Source the ROS setup
echo -e "${GREEN}[${PACKAGE_NAME}] Sourcing ROS workspace...${NC}"
source install/setup.bash

# Launch the package
echo -e "${GREEN}[${PACKAGE_NAME}] Starting...${NC}"
ros2 launch ${PACKAGE_NAME} ${PACKAGE_NAME}.launch.py &

# Capture the PID and create a new process group
LAUNCH_PID=$!

# Wait for the process to finish
wait $LAUNCH_PID
