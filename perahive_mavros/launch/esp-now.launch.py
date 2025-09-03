from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, TimerAction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    # --- Launch arguments ---
    port_arg  = DeclareLaunchArgument('port', default_value='/dev/ttyUSB0')
    baud_arg  = DeclareLaunchArgument('baud', default_value='115200')
    sysid_arg = DeclareLaunchArgument('system_id', default_value='255')

    delay_uav2_arg = DeclareLaunchArgument('delay_uav2_sec', default_value='5.0')
    delay_uav3_arg = DeclareLaunchArgument('delay_uav3_sec', default_value='10.0')
    delay_fg_arg   = DeclareLaunchArgument('delay_foxglove_sec', default_value='15.0')

    # --- Substitutions ---
    port  = LaunchConfiguration('port')
    baud  = LaunchConfiguration('baud')
    sysid = LaunchConfiguration('system_id')

    delay_uav2 = LaunchConfiguration('delay_uav2_sec')
    delay_uav3 = LaunchConfiguration('delay_uav3_sec')
    delay_fg   = LaunchConfiguration('delay_foxglove_sec')

    # --- Common MAVROS params ---
    def mavros_params(target_sys):
        return [{
            'fcu_url': f'serial:///{port.perform(None)}:{baud.perform(None)}',  # resolved at runtime
            'system_id': int(sysid.perform(None)) if sysid.perform(None).isdigit() else sysid,
            'target_system_id': target_sys,
        }]

    # Node factories (must use LaunchConfiguration inside lambdas to defer evaluation)
    uav1 = Node(
        package='mavros',
        executable='mavros_node',
        namespace='uav1',
        output='screen',
        parameters=[{
            'fcu_url': ['serial:///', port, ':', baud],
            'system_id': sysid,
            'target_system_id': 1,
        }],
    )

    uav2 = Node(
        package='mavros',
        executable='mavros_node',
        namespace='uav2',
        output='screen',
        parameters=[{
            'fcu_url': ['serial:///', port, ':', baud],
            'system_id': sysid,
            'target_system_id': 2,
        }],
    )

    uav3 = Node(
        package='mavros',
        executable='mavros_node',
        namespace='uav3',
        output='screen',
        parameters=[{
            'fcu_url': ['serial:///', port, ':', baud],
            'system_id': sysid,
            'target_system_id': 3,
        }],
    )

    # Launch foxglove_bridge directly (avoid XML include)
    foxglove = Node(
        package='foxglove_bridge',
        executable='foxglove_bridge',
        name='foxglove_bridge',
        output='screen',
        # Typical args; change as you need
        arguments=[
            '--port', '8765',
            '--address', '0.0.0.0',
            # '--topics', '/uav1/**,/uav2/**,/uav3/**',   # optional filtering
        ],
    )

    return LaunchDescription([
        port_arg, baud_arg, sysid_arg,
        delay_uav2_arg, delay_uav3_arg, delay_fg_arg,

        # uav1 immediately
        uav1,

        # uav2 after delay_uav2
        TimerAction(period=delay_uav2, actions=[uav2]),

        # uav3 after delay_uav3
        # TimerAction(period=delay_uav3, actions=[uav3]),

        # foxglove after delay_foxglove
        TimerAction(period=delay_fg, actions=[foxglove]),
    ])
