#include <memory>
#include <vector>
#include <string>
#include <map>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "camera_sync_demo/msg/camera_status.hpp"

using CameraStatus = camera_sync_demo::msg::CameraStatus;

/**
 * MaestroNode: Coordinates final synchronization of all cameras
 *
 * Responsibilities:
 * - Listens to camera status messages (via topic subscription)
 * - Tracks which cameras have reached "ready" state
 * - Once all cameras are ready, publishes "start publishing" signal
 * - Cameras listen to this signal to begin synchronized data capture
 */
class MaestroNode : public rclcpp::Node
{
public:
  MaestroNode()
  : Node("maestro_node"),
    expected_cameras_(4),
    all_cameras_ready_(false)
  {
    RCLCPP_INFO(this->get_logger(),
                "Maestro Node started. Waiting for %zu cameras to report ready...",
                expected_cameras_);

    // Subscribe to camera status messages
    camera_status_subscriber_ = this->create_subscription<CameraStatus>(
      "camera_status",
      10,
      [this](const CameraStatus::SharedPtr msg) {
        handle_camera_status(msg);
      });

    // Publisher to signal all cameras to start publishing
    start_publishing_publisher_ = this->create_publisher<std_msgs::msg::Bool>(
      "start_publishing",
      10);
  }

private:
  /**
   * Handles incoming camera status messages
   */
  void handle_camera_status(const CameraStatus::SharedPtr msg)
  {
    const std::string& camera_id = msg->camera_id;
    const std::string& status = msg->status;

    // Log all status updates for visibility
    RCLCPP_INFO(this->get_logger(),
                "Camera '%s' status: %s - %s",
                camera_id.c_str(),
                status.c_str(),
                msg->details.c_str());

    // Track camera readiness
    if (status == "ready") {
      // Add camera to ready set if not already present
      if (ready_cameras_.find(camera_id) == ready_cameras_.end()) {
        ready_cameras_.insert(camera_id);
        camera_ips_[camera_id] = msg->ip_address;

        RCLCPP_INFO(this->get_logger(),
                    "Camera '%s' at %s is READY! (%zu/%zu cameras ready)",
                    camera_id.c_str(),
                    msg->ip_address.c_str(),
                    ready_cameras_.size(),
                    expected_cameras_);

        // Check if all cameras are ready
        check_all_ready();
      }
    }
    else if (status == "error") {
      RCLCPP_ERROR(this->get_logger(),
                   "Camera '%s' reported ERROR: %s",
                   camera_id.c_str(),
                   msg->details.c_str());
    }
  }

  /**
   * Checks if all cameras are ready and signals them to start publishing
   */
  void check_all_ready()
  {
    if (all_cameras_ready_) {
      return;  // Already signaled
    }

    if (ready_cameras_.size() >= expected_cameras_) {
      all_cameras_ready_ = true;

      RCLCPP_INFO(this->get_logger(),
                  "========================================");
      RCLCPP_INFO(this->get_logger(),
                  "ALL %zu CAMERAS READY!",
                  expected_cameras_);
      RCLCPP_INFO(this->get_logger(),
                  "Signaling cameras to start publishing...");
      RCLCPP_INFO(this->get_logger(),
                  "========================================");

      // Log all ready cameras
      for (const auto& camera_id : ready_cameras_) {
        RCLCPP_INFO(this->get_logger(),
                    "  - %s at %s",
                    camera_id.c_str(),
                    camera_ips_[camera_id].c_str());
      }

      // Publish start signal
      auto start_msg = std_msgs::msg::Bool();
      start_msg.data = true;
      start_publishing_publisher_->publish(start_msg);

      RCLCPP_INFO(this->get_logger(),
                  "Start publishing signal sent. System is now synchronized!");
    }
  }

  rclcpp::Subscription<CameraStatus>::SharedPtr camera_status_subscriber_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr start_publishing_publisher_;

  std::set<std::string> ready_cameras_;
  std::map<std::string, std::string> camera_ips_;
  size_t expected_cameras_;
  bool all_cameras_ready_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<MaestroNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
