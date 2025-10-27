#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "camera_sync_demo/action/start_capture.hpp"

using namespace std::chrono_literals;
using StartCapture = camera_sync_demo::action::StartCapture;
using GoalHandleStartCapture = rclcpp_action::ClientGoalHandle<StartCapture>;

/**
 * SyncNode: Initiates capture process for all cameras
 *
 * Responsibilities:
 * - Automatically sends "start capture" commands to all cameras via action calls
 * - Tracks which cameras have acknowledged the start command
 * - This node completes once all cameras confirm they've started
 */
class SyncNode : public rclcpp::Node
{
public:
  SyncNode()
  : Node("sync_node"),
    expected_cameras_(4),
    cameras_started_(0)
  {
    RCLCPP_INFO(this->get_logger(),
                "Sync Node started. Will command %zu cameras to start capture...",
                expected_cameras_);

    // Create action client to send start commands to cameras
    action_client_ = rclcpp_action::create_client<StartCapture>(this, "start_capture");

    // Wait a bit for cameras to come online, then send start commands
    startup_timer_ = this->create_wall_timer(
      2s,
      [this]() {
        startup_timer_->cancel();  // One-time execution
        send_start_commands();
      });
  }

private:
  /**
   * Sends start capture command to all expected cameras
   */
  void send_start_commands()
  {
    RCLCPP_INFO(this->get_logger(), "Waiting for action server...");

    if (!action_client_->wait_for_action_server(10s)) {
      RCLCPP_ERROR(this->get_logger(), "Action server not available!");
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "Sending START CAPTURE commands to all cameras...");

    // Send start command to each camera
    for (size_t i = 1; i <= expected_cameras_; ++i) {
      std::string camera_id = "camera_" + std::to_string(i);
      send_start_to_camera(camera_id);
    }
  }

  /**
   * Sends start capture action goal to a specific camera
   */
  void send_start_to_camera(const std::string& camera_id)
  {
    auto goal = StartCapture::Goal();
    goal.camera_id = camera_id;

    RCLCPP_INFO(this->get_logger(),
                "Commanding camera '%s' to start capture...",
                camera_id.c_str());

    auto send_goal_options = rclcpp_action::Client<StartCapture>::SendGoalOptions();

    // Feedback callback: camera reports initialization progress
    send_goal_options.feedback_callback =
      [this, camera_id](
        GoalHandleStartCapture::SharedPtr,
        const std::shared_ptr<const StartCapture::Feedback> feedback) {
        RCLCPP_INFO(this->get_logger(),
                    "Camera '%s': %s (%d%% complete)",
                    camera_id.c_str(),
                    feedback->initialization_status.c_str(),
                    feedback->progress_percent);
      };

    // Result callback: camera confirms it has started
    send_goal_options.result_callback =
      [this, camera_id](const GoalHandleStartCapture::WrappedResult& result) {
        if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
          cameras_started_++;
          RCLCPP_INFO(this->get_logger(),
                      "Camera '%s' STARTED! (%zu/%zu cameras started) - %s",
                      camera_id.c_str(),
                      cameras_started_,
                      expected_cameras_,
                      result.result->status_message.c_str());

          if (cameras_started_ >= expected_cameras_) {
            RCLCPP_INFO(this->get_logger(),
                        "SUCCESS! All %zu cameras have started capture. "
                        "Maestro will coordinate final synchronization.",
                        expected_cameras_);
          }
        } else {
          RCLCPP_ERROR(this->get_logger(),
                       "Camera '%s' failed to start!",
                       camera_id.c_str());
        }
      };

    action_client_->async_send_goal(goal, send_goal_options);
  }

  rclcpp_action::Client<StartCapture>::SharedPtr action_client_;
  rclcpp::TimerBase::SharedPtr startup_timer_;
  size_t expected_cameras_;
  size_t cameras_started_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SyncNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
