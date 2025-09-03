colcon build --packages-select perahive_mavros && source install/setup.bash && ros2 launch perahive_mavros esp-now.launch.py
ros2 launch foxglove_bridge foxglove_bridge_launch.xml


ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="serial:///dev/ttyUSB0:115200" \
    -p system_id:=255 \
    -p target_system_id:=1 \
    -r __ns:=/uav1 

ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="serial:///dev/ttyUSB0:115200" \
    -p system_id:=255 \
    -p target_system_id:=2 \
    -r __ns:=/uav2 

ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="serial:///dev/ttyUSB0:115200" \
    -p system_id:=255 \
    -p target_system_id:=3 \
    -r __ns:=/uav3


ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="serial:///dev/ttyACM0:115200" \
    -r __ns:=/uav3



    ros2 service call /uav1/set_message_interval mavros_msgs/srv/MessageInterval \
"{message_id: 245, message_rate: 1.0}"


# Simulator 
ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="udp:192.168.184.73:14550" \
    -r __ns:=/uav1

ros2 run mavros mavros_node --ros-args \
    -p fcu_url:="udp:192.168.184.73:14551" \
    -r __ns:=/uav2