# Maestro Demo

### Commands to Start
```bash
docker-compose run --rm ros2_dev
```

### Commands inside docker terminal
```bash
colcon build --packages-select camera_sync_demo
source install/setup.bash
ros2 launch camera_sync_demo camera_sync_demo.launch.py
```