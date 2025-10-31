#include <rclcpp/rclcpp.hpp>
#include <mavros_msgs/srv/command_bool.hpp>
#include <mavros_msgs/srv/command_tol.hpp>
#include <mavros_msgs/srv/set_mode.hpp>
#include "perahive_interfaces/srv/launch_drone.hpp"
#include <chrono>
#include <functional>
#include <future>
#include <thread>

using namespace std::chrono_literals;
using perahive_interfaces::srv::LaunchDrone;

class LaunchSequenceNode : public rclcpp::Node
{
public:
  LaunchSequenceNode() : Node("launch_sequence")
  {
    // Callback groups
    service_group_ = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    client_group_  = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);

    // Use Services QoS + callback groups (no deprecated rmw struct, no Options)
    arming_client_ = this->create_client<mavros_msgs::srv::CommandBool>(
        "/mavros/cmd/arming",
        rclcpp::ServicesQoS(),
        client_group_);

    takeoff_client_ = this->create_client<mavros_msgs::srv::CommandTOL>(
        "/mavros/cmd/takeoff",
        rclcpp::ServicesQoS(),
        client_group_);

    set_mode_client_ = this->create_client<mavros_msgs::srv::SetMode>(
        "/mavros/set_mode",
        rclcpp::ServicesQoS(),
        client_group_);


    RCLCPP_INFO(get_logger(), "Waiting for MAVROS services...");
    arming_client_->wait_for_service(10s);
    takeoff_client_->wait_for_service(10s);
    set_mode_client_->wait_for_service(10s);
    RCLCPP_INFO(get_logger(), "MAVROS services are available.");

    // Our service (QoS + callback group)
    launch_service_ = this->create_service<perahive_interfaces::srv::LaunchDrone>(
        "launch_drone",
        std::bind(&LaunchSequenceNode::launch_callback, this,
                std::placeholders::_1, std::placeholders::_2),
        rclcpp::ServicesQoS(),
        service_group_);
        
    RCLCPP_INFO(get_logger(), "Service '/launch_drone' ready.");
  }

private:
  // --- Generic "sync call" helper: waits sequentially with timeout -------------------------
  template<class ServiceT, class RequestT>
  bool call_sync(const typename rclcpp::Client<ServiceT>::SharedPtr & client,
                 const std::shared_ptr<RequestT> & req,
                 std::chrono::milliseconds timeout,
                 std::function<bool(const std::shared_ptr<typename ServiceT::Response> &)> ok_pred,
                 const char* name)
  {
    auto fut = client->async_send_request(req);
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (rclcpp::ok()) {
      if (fut.wait_for(100ms) == std::future_status::ready) {
        auto res = fut.get();
        bool ok = (res && ok_pred(res));
        RCLCPP_INFO(get_logger(), "%s %s", name, ok ? "OK" : "FAILED");
        return ok;
      }
      if (std::chrono::steady_clock::now() > deadline) {
        RCLCPP_ERROR(get_logger(), "%s timed out", name);
        return false;
      }
      // Let other work progress
      std::this_thread::sleep_for(10ms);
    }
    return false;
  }

  // --- Step 1: set GUIDED ---------------------------------------------------------------
  bool set_guided_mode()
  {
    auto req = std::make_shared<mavros_msgs::srv::SetMode::Request>();
    req->base_mode = 0;
    req->custom_mode = "GUIDED";
    return call_sync<mavros_msgs::srv::SetMode>(
      set_mode_client_, req, 10s,
      [](const std::shared_ptr<mavros_msgs::srv::SetMode::Response> & r){ return r->mode_sent; },
      "SetMode(GUIDED)");
  }

  // --- Step 2: arm ----------------------------------------------------------------------
  bool arm_vehicle()
  {
    auto req = std::make_shared<mavros_msgs::srv::CommandBool::Request>();
    req->value = true;
    return call_sync<mavros_msgs::srv::CommandBool>(
      arming_client_, req, 10s,
      [](const std::shared_ptr<mavros_msgs::srv::CommandBool::Response> & r){ return r->success; },
      "Arming");
  }

  // --- Step 3: takeoff ------------------------------------------------------------------
  bool takeoff(float altitude)
  {
    auto req = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();
    req->altitude = altitude;
    req->latitude = 0.0;
    req->longitude = 0.0;
    req->min_pitch = 0.0;
    req->yaw = 0.0;
    return call_sync<mavros_msgs::srv::CommandTOL>(
      takeoff_client_, req, 15s,
      [](const std::shared_ptr<mavros_msgs::srv::CommandTOL::Response> & r){ return r->success; },
      "Takeoff");
  }

  // --- Service callback: linear, early-return on failure --------------------------------
  void launch_callback(const std::shared_ptr<LaunchDrone::Request> request,
                       std::shared_ptr<LaunchDrone::Response> response)
  {
    const float altitude = request->altitude;
    RCLCPP_INFO(get_logger(), "Launch requested to %.2f m", altitude);

    if (!set_guided_mode()) {
      response->success = false; response->message = "Failed to set GUIDED mode"; return;
    }
    if (!arm_vehicle()) {
      response->success = false; response->message = "Failed to arm vehicle"; return;
    }
    if (!takeoff(altitude)) {
      response->success = false; response->message = "Failed to take off"; return;
    }

    response->success = true;
    response->message = "Drone launched successfully";
  }

  // Clients & service
  rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedPtr arming_client_;
  rclcpp::Client<mavros_msgs::srv::CommandTOL>::SharedPtr  takeoff_client_;
  rclcpp::Client<mavros_msgs::srv::SetMode>::SharedPtr     set_mode_client_;
  rclcpp::Service<LaunchDrone>::SharedPtr launch_service_;

  // Callback groups
  rclcpp::CallbackGroup::SharedPtr service_group_;
  rclcpp::CallbackGroup::SharedPtr client_group_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<LaunchSequenceNode>();

  rclcpp::executors::MultiThreadedExecutor exec(rclcpp::ExecutorOptions(), 2);
  exec.add_node(node);
  exec.spin();

  rclcpp::shutdown();
  return 0;
}