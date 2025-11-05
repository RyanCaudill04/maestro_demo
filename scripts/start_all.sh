#!/bin/bash

# Master script to launch all ROS packages
# This script starts each package in its own process and manages their lifecycle

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Array to store PIDs of all launched scripts
declare -a PIDS=()
declare -a PACKAGE_NAMES=()

# Cleanup function - called on script exit
cleanup() {
    echo -e "\n${YELLOW}========================================${NC}"
    echo -e "${YELLOW}Shutting down all packages...${NC}"
    echo -e "${YELLOW}========================================${NC}"

    # Kill all child processes
    for i in "${!PIDS[@]}"; do
        local pid="${PIDS[$i]}"
        local name="${PACKAGE_NAMES[$i]}"

        if kill -0 "$pid" 2>/dev/null; then
            echo -e "${YELLOW}Stopping ${name} (PID: ${pid})...${NC}"
            kill -SIGTERM "$pid" 2>/dev/null || true
        fi
    done

    # Wait a bit for graceful shutdown
    sleep 3

    # Force kill any remaining processes
    for i in "${!PIDS[@]}"; do
        local pid="${PIDS[$i]}"
        local name="${PACKAGE_NAMES[$i]}"

        if kill -0 "$pid" 2>/dev/null; then
            echo -e "${RED}Force killing ${name} (PID: ${pid})...${NC}"
            kill -SIGKILL "$pid" 2>/dev/null || true
        fi
    done

    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}All packages stopped${NC}"
    echo -e "${GREEN}========================================${NC}"
}

# Register cleanup function for various signals
trap cleanup EXIT SIGINT SIGTERM

# Change to workspace root
cd "$WORKSPACE_ROOT"

# Check if workspace is built
if [ ! -f "install/setup.bash" ]; then
    echo -e "${RED}Error: Workspace not built${NC}"
    echo "Please run 'colcon build' first"
    exit 1
fi

# Print header
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Starting all ROS packages${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Launch camera_sync_demo
echo -e "${GREEN}[1/2] Launching camera_sync_demo...${NC}"
bash "$SCRIPT_DIR/camera_sync_demo.sh" &
CAMERA_PID=$!
PIDS+=($CAMERA_PID)
PACKAGE_NAMES+=("camera_sync_demo")
echo -e "${GREEN}Started camera_sync_demo (PID: ${CAMERA_PID})${NC}"
echo ""

# Give it a moment to initialize
sleep 1

# Launch imu_processor
echo -e "${GREEN}[2/2] Launching imu_processor...${NC}"
bash "$SCRIPT_DIR/imu_processor.sh" &
IMU_PID=$!
PIDS+=($IMU_PID)
PACKAGE_NAMES+=("imu_processor")
echo -e "${GREEN}Started imu_processor (PID: ${IMU_PID})${NC}"
echo ""

# Print summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}All packages started successfully${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Running processes:${NC}"
for i in "${!PIDS[@]}"; do
    echo -e "  - ${PACKAGE_NAMES[$i]}: PID ${PIDS[$i]}"
done
echo ""
echo -e "${YELLOW}Press Ctrl+C to stop all processes${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Wait for all background processes
# This will keep the script running until one exits or we receive a signal
wait
