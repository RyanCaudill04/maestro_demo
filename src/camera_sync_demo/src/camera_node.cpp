#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "camera_sync_demo/action/camera_ready.hpp"

using namespace std::chrono_literals;
using CameraReady = camera_sync_demo::action::CameraReady;
using GoalHandleCameraReady = rclcpp_action::ClientGoalHandle<CameraReady>;

class CameraNode : public rclcpp::Node
{
public:
  CameraNode(const std::string& camera_id, const std::string& ip_address, int startup_time_ms)
  : Node(camera_id),
    camera_id_(camera_id),
    ip_address_(ip_address),
    startup_time_ms_(startup_time_ms),
    is_synchronized_(false)
  {
    RCLCPP_INFO(this->get_logger(), "Camera '%s' at %s initializing...",
                camera_id_.c_str(), ip_address_.c_str());

    // Create action client to communicate with sync coordinator
    action_client_ = rclcpp_action::create_client<CameraReady>(this, "camera_sync");

    // Create publisher for camera images (simulated)
    image_publisher_ = this->create_publisher<sensor_msgs::msg::Image>(
      camera_id_ + "/image_raw", 10);

    // Start initialization process
    init_thread_ = std::thread(&CameraNode::initialize, this);
  }

  ~CameraNode()
  {
    if (init_thread_.joinable()) {
      init_thread_.join();
    }
  }

private:
  void initialize()
  {
    // Simulate variable startup time
    RCLCPP_INFO(this->get_logger(), "Camera '%s' starting initialization (%d ms)...",
                camera_id_.c_str(), startup_time_ms_);
    std::this_thread::sleep_for(std::chrono::milliseconds(startup_time_ms_));

    RCLCPP_INFO(this->get_logger(), "Camera '%s' initialization complete! Waiting for sync server...",
                camera_id_.c_str());

    // Wait for action server to be available
    if (!action_client_->wait_for_action_server(30s)) {
      RCLCPP_ERROR(this->get_logger(), "Sync coordinator not available!");
      return;
    }

    // Send ready signal to coordinator
    send_ready_goal();
  }

  void send_ready_goal()
  {
    auto goal_msg = CameraReady::Goal();
    goal_msg.camera_id = camera_id_;
    goal_msg.ip_address = ip_address_;

    RCLCPP_INFO(this->get_logger(), "Camera '%s' sending ready signal...", camera_id_.c_str());

    auto send_goal_options = rclcpp_action::Client<CameraReady>::SendGoalOptions();

    send_goal_options.feedback_callback =
      [this](GoalHandleCameraReady::SharedPtr,
             const std::shared_ptr<const CameraReady::Feedback> feedback) {
        RCLCPP_INFO(this->get_logger(),
                    "Camera '%s': %s (%d/%d cameras ready)",
                    camera_id_.c_str(),
                    feedback->status_message.c_str(),
                    feedback->cameras_ready_count,
                    4); // Expected 4 cameras
      };

    send_goal_options.result_callback =
      [this](const GoalHandleCameraReady::WrappedResult& result) {
        if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
          RCLCPP_INFO(this->get_logger(),
                      "Camera '%s': ALL CAMERAS READY! Starting synchronized operation...",
                      camera_id_.c_str());
          is_synchronized_ = true;
          start_publishing();
        } else {
          RCLCPP_ERROR(this->get_logger(), "Camera '%s': Goal failed!", camera_id_.c_str());
        }
      };

    action_client_->async_send_goal(goal_msg, send_goal_options);
  }

  void start_publishing()
  {
    // Start publishing images at 30 Hz
    timer_ = this->create_wall_timer(
      33ms,
      [this]() {
        auto msg = sensor_msgs::msg::Image();
        msg.header.stamp = this->now();
        msg.header.frame_id = camera_id_;
        msg.height = 480;
        msg.width = 640;
        msg.encoding = "rgb8";

        RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                            "Camera '%s' publishing frame", camera_id_.c_str());

        image_publisher_->publish(msg);
      });
  }

  std::string camera_id_;
  std::string ip_address_;
  int startup_time_ms_;
  bool is_synchronized_;

  rclcpp_action::Client<CameraReady>::SharedPtr action_client_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::thread init_thread_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  if (argc < 4) {
    std::cerr << "Usage: camera_node <camera_id> <ip_address> <startup_time_ms>" << std::endl;
    return 1;
  }

  std::string camera_id = argv[1];
  std::string ip_address = argv[2];
  int startup_time = std::stoi(argv[3]);

  auto node = std::make_shared<CameraNode>(camera_id, ip_address, startup_time);
  rclcpp::spin(node);
  rclcpp::shutdown();

  return 0;
}
