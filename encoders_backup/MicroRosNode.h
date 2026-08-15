#ifndef MICRO_ROS_NODE_H
#define MICRO_ROS_NODE_H

#include <Arduino.h>
#include "config.h"
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <sensor_msgs/msg/joint_state.h>

class MicroRosNode {
public:
    MicroRosNode();

    /**
     * Initializes the micro-ROS transport and node.
     * Blocks until the agent is reachable.
     */
    void init();

    /**
     * Publishes the joint state message with the current angles.
     * @param angles Array of encoder angles in radians.
     */
    void publishAngles(float* angles);

    /**
     * Spins the micro-ROS executor to handle callbacks (not strictly needed for just publishing,
     * but good practice in case subscribers are added later).
     */
    void spinSome();

private:
    rcl_publisher_t publisher;
    sensor_msgs__msg__JointState msg;
    rclc_executor_t executor;
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t node;
    
    // Arrays for the JointState message
    rosidl_runtime_c__String name_array[NUM_ENCODERS];
    char name_strings[NUM_ENCODERS][20]; // Buffer for strings like "joint_1"
    double position_array[NUM_ENCODERS];

    void error_loop();
};

#endif // MICRO_ROS_NODE_H
