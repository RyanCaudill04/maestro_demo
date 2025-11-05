# Maestro Demo

### Commands to Start
```bash
docker-compose run --rm ros2_dev
```

### Commands inside docker terminal

#### Build All Packages
```bash
colcon build
source install/setup.bash
```

#### Using Launch Scripts (Recommended)

**Start all packages at once:**
```bash
./scripts/start_all.sh
```
This launches all packages in separate processes. Press `Ctrl+C` to stop all packages.

**Start individual packages:**
```bash
# Camera sync demo only
./scripts/camera_sync_demo.sh

# IMU processor only
./scripts/imu_processor.sh
```

#### Manual Launch (Alternative)

**Camera Sync Demo:**
```bash
colcon build --packages-select camera_sync_demo
source install/setup.bash
ros2 launch camera_sync_demo camera_sync_demo.launch.py
```

**IMU Processor:**
```bash
colcon build --packages-select imu_processor
source install/setup.bash
ros2 launch imu_processor imu_processor.launch.py
```