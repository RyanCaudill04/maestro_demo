#include <memory>
#include <vector>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "camera_sync_demo/action/camera_ready.hpp"

using CameraReady = camera_sync_demo::action::CameraReady;
using GoalHandleCameraReady = rclcpp_action::ServerGoalHandle<CameraReady>;

class SyncCoordinator : public rclcpp::Node
{
public:
  SyncCoordinator()
  : Node("sync_coordinator"),
    expected_cameras_(4),
    all_ready_(false)
  {
    RCLCPP_INFO(this->get_logger(), "Sync Coordinator started. Waiting for %zu cameras...",
                expected_cameras_);

    // Create action server
    action_server_ = rclcpp_action::create_server<CameraReady>(
      this,
      "camera_sync",
      [this](const auto& uuid, const auto& goal) { return handle_goal(uuid, goal); },
      [this](const auto& goal_handle) { return handle_cancel(goal_handle); },
      [this](const auto& goal_handle) { handle_accepted(goal_handle); });
  }

private:
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID& uuid,
    std::shared_ptr<const CameraReady::Goal> goal)
  {
    (void)uuid;
    RCLCPP_INFO(this->get_logger(), "Received ready signal from camera '%s' at %s",
                goal->camera_id.c_str(), goal->ip_address.c_str());

    // Accept all camera goals
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleCameraReady> goal_handle)
  {
    (void)goal_handle;
    RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleCameraReady> goal_handle)
  {
    // Execute goal in a separate thread
    std::thread{[this](const auto& goal_handle) { execute(goal_handle); }, goal_handle}.detach();
  }

  void execute(const std::shared_ptr<GoalHandleCameraReady> goal_handle)
  {
    const auto goal = goal_handle->get_goal();
    auto feedback = std::make_shared<CameraReady::Feedback>();
    auto result = std::make_shared<CameraReady::Result>();

    // Add camera to ready list
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (std::find(ready_cameras_.begin(), ready_cameras_.end(), goal->camera_id) == ready_cameras_.end()) {
        ready_cameras_.push_back(goal->camera_id);
        camera_ips_[goal->camera_id] = goal->ip_address;

        RCLCPP_INFO(this->get_logger(), "Camera '%s' added to ready list (%zu/%zu ready)",
                    goal->camera_id.c_str(),
                    ready_cameras_.size(),
                    expected_cameras_);
      }
    }

    // Wait for all cameras to be ready
    rclcpp::Rate loop_rate(2); // Check every 500ms
    while (rclcpp::ok()) {
      bool all_cameras_ready = false;

      {
        std::lock_guard<std::mutex> lock(mutex_);

        // Send feedback
        feedback->cameras_ready_count = static_cast<int>(ready_cameras_.size());
        feedback->status_message = "Waiting for " +
                                  std::to_string(expected_cameras_ - ready_cameras_.size()) +
                                  " more camera(s)";
        goal_handle->publish_feedback(feedback);

        RCLCPP_INFO(this->get_logger(), "Status: %d/%zu cameras ready",
                    static_cast<int>(ready_cameras_.size()), expected_cameras_);

        // Check if all cameras are ready
        if (ready_cameras_.size() >= expected_cameras_) {
          all_ready_ = true;
          all_cameras_ready = true;

          // Prepare result
          result->all_cameras_ready = true;
          result->total_cameras = static_cast<int>(ready_cameras_.size());
          result->camera_list = ready_cameras_;

          RCLCPP_INFO(this->get_logger(), "ALL %zu CAMERAS READY! Signaling synchronized start...",
                      expected_cameras_);
        }
      } // Release mutex here before succeed/break

      if (all_cameras_ready) {
        goal_handle->succeed(result);
        break;
      }

      loop_rate.sleep();
    }
  }

  rclcpp_action::Server<CameraReady>::SharedPtr action_server_;
  std::vector<std::string> ready_cameras_;
  std::map<std::string, std::string> camera_ips_;
  size_t expected_cameras_;
  bool all_ready_;
  std::mutex mutex_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SyncCoordinator>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
