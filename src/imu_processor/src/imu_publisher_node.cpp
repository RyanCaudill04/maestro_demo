#include <chrono>
#include <memory>
#include <random>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

using namespace std::chrono_literals;

class IMUPublisherNode : public rclcpp::Node
{
public:
  IMUPublisherNode()
  : Node("imu_publisher_node"),
    gen_(rd_()),
    noise_dist_(0.0, 0.05),
    angle_(0.0)
  {
    // Create publisher for IMU data
    publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);

    // Create timer to publish at 50Hz
    timer_ = this->create_wall_timer(
      20ms, std::bind(&IMUPublisherNode::publishIMUData, this));

    RCLCPP_INFO(this->get_logger(), "IMU Publisher Node started - Publishing at 50Hz on /imu/data");
  }

private:
  void publishIMUData()
  {
    auto msg = sensor_msgs::msg::Imu();

    // Set timestamp
    msg.header.stamp = this->now();
    msg.header.frame_id = "imu_link";

    // Simulate a rotating device with some linear acceleration
    angle_ += 0.02; // Increment angle for simulation

    // Linear acceleration (m/s^2) - simulate slight movement with gravity
    msg.linear_acceleration.x = 0.5 * std::sin(angle_) + noise_dist_(gen_);
    msg.linear_acceleration.y = 0.5 * std::cos(angle_) + noise_dist_(gen_);
    msg.linear_acceleration.z = 9.81 + noise_dist_(gen_); // Gravity + noise

    // Angular velocity (rad/s) - simulate rotation
    msg.angular_velocity.x = 0.1 * std::sin(angle_ * 0.5) + noise_dist_(gen_);
    msg.angular_velocity.y = 0.1 * std::cos(angle_ * 0.5) + noise_dist_(gen_);
    msg.angular_velocity.z = 0.2 + noise_dist_(gen_);

    // Orientation (quaternion) - simulate orientation from rotation
    double roll = 0.1 * std::sin(angle_);
    double pitch = 0.1 * std::cos(angle_);
    double yaw = angle_ * 0.5;

    // Convert euler to quaternion (simplified)
    double cy = std::cos(yaw * 0.5);
    double sy = std::sin(yaw * 0.5);
    double cp = std::cos(pitch * 0.5);
    double sp = std::sin(pitch * 0.5);
    double cr = std::cos(roll * 0.5);
    double sr = std::sin(roll * 0.5);

    msg.orientation.w = cr * cp * cy + sr * sp * sy;
    msg.orientation.x = sr * cp * cy - cr * sp * sy;
    msg.orientation.y = cr * sp * cy + sr * cp * sy;
    msg.orientation.z = cr * cp * sy - sr * sp * cy;

    // Set covariance matrices (diagonal for simplicity)
    for (int i = 0; i < 9; i++) {
      msg.orientation_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
      msg.angular_velocity_covariance[i] = (i % 4 == 0) ? 0.001 : 0.0;
      msg.linear_acceleration_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
    }

    publisher_->publish(msg);
  }

  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;

  // Random number generation for noise
  std::random_device rd_;
  std::mt19937 gen_;
  std::normal_distribution<double> noise_dist_;

  // Simulation state
  double angle_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<IMUPublisherNode>());
  rclcpp::shutdown();
  return 0;
}
