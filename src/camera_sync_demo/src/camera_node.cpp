#include <chrono>
#include <memory>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/bool.hpp"
#include "camera_sync_demo/action/start_capture.hpp"
#include "camera_sync_demo/msg/camera_status.hpp"

using namespace std::chrono_literals;
using StartCapture = camera_sync_demo::action::StartCapture;
using GoalHandleStartCapture = rclcpp_action::ServerGoalHandle<StartCapture>;
using CameraStatus = camera_sync_demo::msg::CameraStatus;

/**
 * CameraNode: Simulates a camera in the synchronized capture system
 *
 * Flow:
 * 1. Receives "start capture" command from sync node (via action)
 * 2. Simulates initialization/startup delay
 * 3. Publishes "ready" status to maestro node (via topic)
 * 4. Waits for maestro's "start publishing" signal (via topic)
 * 5. Begins publishing image data
 */
class CameraNode : public rclcpp::Node
{
public:
  CameraNode(const std::string& camera_id, const std::string& ip_address, int startup_time_ms)
  : Node(camera_id),
    camera_id_(camera_id),
    ip_address_(ip_address),
    startup_time_ms_(startup_time_ms),
    is_publishing_(false)
  {
    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' at %s initializing (startup time: %d ms)...",
                camera_id_.c_str(),
                ip_address_.c_str(),
                startup_time_ms_);

    // Create action server to receive start commands from sync node
    action_server_ = rclcpp_action::create_server<StartCapture>(
      this,
      "start_capture",
      [this](const auto& uuid, auto goal) { return handle_goal(uuid, goal); },
      [this](const auto& goal_handle) { return handle_cancel(goal_handle); },
      [this](const auto& goal_handle) { handle_accepted(goal_handle); });

    // Publisher for status messages to maestro
    status_publisher_ = this->create_publisher<CameraStatus>("camera_status", 10);

    // Subscriber for start publishing signal from maestro
    start_publishing_subscriber_ = this->create_subscription<std_msgs::msg::Bool>(
      "start_publishing",
      10,
      [this](const std_msgs::msg::Bool::SharedPtr msg) {
        handle_start_publishing(msg);
      });

    // Publisher for camera images (created but not used until signal received)
    image_publisher_ = this->create_publisher<sensor_msgs::msg::Image>(
      camera_id_ + "/image_raw",
      10);

    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' waiting for start capture command...",
                camera_id_.c_str());
  }

  ~CameraNode()
  {
    if (init_thread_.joinable()) {
      init_thread_.join();
    }
  }

private:
  /**
   * Action server callbacks: Handle start capture commands
   */
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID& uuid,
    std::shared_ptr<const StartCapture::Goal> goal)
  {
    (void)uuid;

    // Only accept goal for this specific camera
    if (goal->camera_id == camera_id_) {
      RCLCPP_INFO(this->get_logger(),
                  "Camera '%s' received START CAPTURE command!",
                  camera_id_.c_str());
      return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    } else {
      return rclcpp_action::GoalResponse::REJECT;
    }
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleStartCapture> goal_handle)
  {
    (void)goal_handle;
    RCLCPP_INFO(this->get_logger(), "Received cancel request");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleStartCapture> goal_handle)
  {
    // Execute initialization in separate thread
    init_thread_ = std::thread(
      [this, goal_handle]() { execute_initialization(goal_handle); });
    init_thread_.detach();
  }

  /**
   * Simulates camera initialization process
   */
  void execute_initialization(const std::shared_ptr<GoalHandleStartCapture> goal_handle)
  {
    const auto goal = goal_handle->get_goal();
    auto feedback = std::make_shared<StartCapture::Feedback>();
    auto result = std::make_shared<StartCapture::Result>();

    // Publish initializing status
    publish_status("initializing", "Camera hardware starting up...");

    // Simulate variable startup time with progress updates
    const int steps = 5;
    const int step_duration_ms = startup_time_ms_ / steps;

    for (int i = 1; i <= steps; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(step_duration_ms));

      // Send feedback to sync node
      feedback->progress_percent = (i * 100) / steps;
      feedback->initialization_status = "Initializing hardware (" +
                                       std::to_string(feedback->progress_percent) + "%)";
      goal_handle->publish_feedback(feedback);

      RCLCPP_INFO(this->get_logger(),
                  "Camera '%s' initialization progress: %d%%",
                  camera_id_.c_str(),
                  feedback->progress_percent);
    }

    // Initialization complete - send result to sync node
    result->capture_started = true;
    result->camera_id = camera_id_;
    result->status_message = "Camera initialized and ready";
    goal_handle->succeed(result);

    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' initialization COMPLETE!",
                camera_id_.c_str());

    // Publish ready status to maestro
    publish_status("ready", "Waiting for synchronized start signal...");
  }

  /**
   * Handles start publishing signal from maestro
   */
  void handle_start_publishing(const std_msgs::msg::Bool::SharedPtr msg)
  {
    if (msg->data && !is_publishing_) {
      is_publishing_ = true;

      RCLCPP_INFO(this->get_logger(),
                  "Camera '%s' received START PUBLISHING signal!",
                  camera_id_.c_str());

      publish_status("capturing", "Now publishing synchronized images");
      start_publishing_images();
    }
  }

  /**
   * Publishes status messages to maestro node
   */
  void publish_status(const std::string& status, const std::string& details)
  {
    auto status_msg = CameraStatus();
    status_msg.camera_id = camera_id_;
    status_msg.ip_address = ip_address_;
    status_msg.status = status;
    status_msg.details = details;

    status_publisher_->publish(status_msg);

    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' status: %s - %s",
                camera_id_.c_str(),
                status.c_str(),
                details.c_str());
  }

  /**
   * Starts publishing images at 30 Hz
   */
  void start_publishing_images()
  {
    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' starting synchronized image publishing at 30 Hz...",
                camera_id_.c_str());

    // Create 30Hz timer for image publishing
    image_timer_ = this->create_wall_timer(
      33ms,
      [this]() {
        auto msg = sensor_msgs::msg::Image();
        msg.header.stamp = this->now();
        msg.header.frame_id = camera_id_;
        msg.height = 480;
        msg.width = 640;
        msg.encoding = "rgb8";

        RCLCPP_INFO_THROTTLE(
          this->get_logger(),
          *this->get_clock(),
          1000,  // Log every 1000ms
          "Camera '%s' publishing synchronized frame",
          camera_id_.c_str());

        image_publisher_->publish(msg);
      });
  }

  // Camera configuration
  std::string camera_id_;
  std::string ip_address_;
  int startup_time_ms_;
  bool is_publishing_;

  // ROS2 communication objects
  rclcpp_action::Server<StartCapture>::SharedPtr action_server_;
  rclcpp::Publisher<CameraStatus>::SharedPtr status_publisher_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr start_publishing_subscriber_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
  rclcpp::TimerBase::SharedPtr image_timer_;

  // Threading
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
