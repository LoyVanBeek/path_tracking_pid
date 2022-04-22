#include <path_tracking_pid/path_tracking_pid_local_planner.hpp>
#include <ros/ros.h>
#include <ros/console.h>
#include <geometry_msgs/PoseStamped.h>
// #include <tf2/transform_listener.h>
#include <tf2_ros/transform_listener.h>
#include <costmap_2d/costmap_2d_ros.h>


int main(int argc, char *argv[])
{
  ros::init(argc, argv, "path_tracker");
  ros::NodeHandle nh;
  ROS_INFO("Hello Path Tracker");

  auto tplp = new path_tracking_pid::TrackingPidLocalPlanner();

  // Call initialize with empty costmap that we don't update
  // Subscribe to Paths, convert those to vector of PoseStamped for setPlan
  // Call computeVelocities at some configurable rate?

  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener tfListener(tfBuffer);
  // tf2::TransformListener listener;

  costmap_2d::Costmap2DROS costmap("empty_costmap", tfBuffer);

  std::string map_frame_;
  std::string base_link_frame_;

  nh.param<std::string>("base_link_frame", base_link_frame_, "base_link");
  map_frame_ = costmap.getGlobalFrameID(); //TODO: Not sure if this works if we init the costmap like this
  ROS_INFO_STREAM("base_link_frame=" << base_link_frame_ << ", map_frame=" << map_frame_);

  tf2::Stamped<tf2::Transform> prev_transform;
  // listener.lookupTransform(map_frame_, base_link_frame_, ros::Time(0), prev_transform);

  ros::Publisher cmd_vel_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 5);

  auto receivePath = [&](const nav_msgs::PathConstPtr &path)
  {
    ROS_INFO_STREAM("Received a path of length " << path->poses.size());
    tplp->setPlan(path->poses);
  };
  auto sub = nh.subscribe<nav_msgs::Path>("path", 1, receivePath);

  tplp->initialize("path_tracker", &tfBuffer, &costmap);

  auto timerCallback = [&](const ros::TimerEvent &)
  {
    ROS_INFO("Tick");
    /* TODO:
     * - Get the current robot pose from TF
     * - Calculate velocity from diff with previous pose
     * - publish the Twist
     */

    geometry_msgs::TwistStamped cmd_vel;
    geometry_msgs::PoseStamped current_pose;
    geometry_msgs::TwistStamped velocity;
    std::string message;

    tplp->computeVelocityCommands(current_pose, velocity, cmd_vel, message);

    ROS_INFO_COND(!message.empty(), message.c_str());

    cmd_vel_pub.publish(cmd_vel.twist);
  };
  ros::Timer timer = nh.createTimer(ros::Duration(0.1), timerCallback);

  ros::spin();

  tplp->cancel();

  ROS_INFO("Bye-bye Path Tracker");

  return 0;
}