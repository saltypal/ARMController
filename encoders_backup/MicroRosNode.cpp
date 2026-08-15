#include "MicroRosNode.h"

// Error handle loop
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

MicroRosNode::MicroRosNode() {
    // Initialize the JointState message memory
    msg.name.capacity = NUM_ENCODERS;
    msg.name.size = NUM_ENCODERS;
    msg.name.data = name_array;
    
    msg.position.capacity = NUM_ENCODERS;
    msg.position.size = NUM_ENCODERS;
    msg.position.data = position_array;
    
    // Initialize joint names
    for (int i = 0; i < NUM_ENCODERS; i++) {
        sprintf(name_strings[i], "joint_%d", i + 1);
        name_array[i].data = (char*)name_strings[i];
        name_array[i].size = strlen(name_strings[i]);
        name_array[i].capacity = 20;
    }
}

void MicroRosNode::error_loop() {
    while(1) {
        delay(100);
        // Maybe blink an LED here if one is available
    }
}

void MicroRosNode::init() {
    set_microros_serial_transports(Serial);
    
    // Wait for the agent to be available
    allocator = rcl_get_default_allocator();
    
    // Create init_options
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    // Create node
    RCCHECK(rclc_node_init_default(&node, ROS_NODE_NAME, "", &support));

    // Create publisher
    RCCHECK(rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
        ROS_TOPIC_NAME));

    // Create executor (optional if only publishing, but good for future expansion)
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    
    // We don't have any subscriptions right now, so executor spinning is mostly a no-op
}

void MicroRosNode::publishAngles(float* angles) {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        // ROS 2 JointState uses double for position
        msg.position.data[i] = (double)angles[i];
    }
    
    // Update timestamp (this requires synchronized time with the agent, 
    // which micro-ROS handles via rmw_uros_sync_session, but for simplicity 
    // we let the subscriber side handle stamp if it's 0, or we can use rmw_uros_epoch_nanos)
    // Here we just publish what we have.
    
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
}

void MicroRosNode::spinSome() {
    RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
}
