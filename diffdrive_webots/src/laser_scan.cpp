#include "diffdrive_webots/laser_scan.hpp"
#include <sensor_msgs/image_encodings.hpp>

using std::placeholders::_1;

namespace diffdrive_webots_plugin
{
  void LaserScan::init(webots_ros2_driver::WebotsNode *node, std::unordered_map<std::string, std::string> &parameters)
  {
    // initialize member variables
    node_ = node;
    robot_ = node->robot();
    lidar_ = NULL;

    // retrieve tags
    if (parameters.count("laserScanPeriodMs"))
      lidar_period_ = std::stoi(parameters["laserScanPeriodMs"]);
    else
      throw std::runtime_error("Must set laserScanPeriodMs tag");

    std::string laser_scan_name;
    if (parameters.count("laserScanName"))
      laser_scan_name = parameters["laserScanName"];
    else
      throw std::runtime_error("Must set laserScanName tag");
    
    std::string frame_id;
    if (parameters.count("frameID"))
      frame_id = parameters["frameID"];
    else
      throw std::runtime_error("Must set frameID tag");

    // set webots device
    lidar_ = robot_->getLidar(laser_scan_name);
    if (lidar_ == NULL)
      throw std::runtime_error("Cannot find laserScan with name " + laser_scan_name);
    
    
    int timestep = (int)robot_->getBasicTimeStep();
    if (lidar_period_ % timestep != 0)
      throw std::runtime_error("laserScanPeriodMs must be integer multiple of basicTimeStep");

    lidar_->enable(lidar_period_);

    scan_.header.frame_id = frame_id;
    const int resolution = lidar_->getHorizontalResolution();
    scan_.angle_increment = -lidar_->getFov() / resolution;
    scan_.angle_min = lidar_->getFov() / 2.0 - scan_.angle_increment;
    scan_.angle_max =-lidar_->getFov() / 2.0;
    scan_.range_min = lidar_->getMinRange();
    scan_.range_max = lidar_->getMaxRange();
    scan_.time_increment = (double)lidar_period_ / (1e3 * resolution);
    scan_.scan_time = (double)lidar_period_ / 1e3;
    scan_.ranges.resize(resolution);

    scan_pub_ = node->create_publisher<sensor_msgs::msg::LaserScan>("scan", rclcpp::SensorDataQoS().reliable());
  }

  void LaserScan::step()
  {
    int64_t sim_time = (int64_t)(robot_->getTime() * 1e3);

    if (sim_time % lidar_period_ == 0)
    {
      scan_.header.stamp = node_->get_clock()->now();
      memcpy(scan_.ranges.data(), lidar_->getLayerRangeImage(0), scan_.ranges.size() * sizeof(float));
      scan_pub_->publish(scan_);
    }

  }

}

// The class has to be exported with `PLUGINLIB_EXPORT_CLASS` macro.
// The first argument is the name of your class, while the second is always `webots_ros2_driver::PluginInterface`
#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(diffdrive_webots_plugin::LaserScan, webots_ros2_driver::PluginInterface)