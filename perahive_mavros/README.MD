# perahive_mavros

`perahive_mavros` is a ROS 2 package inside **perahive-core** that provides
ready-to-use launch files to start one or more **MAVROS** nodes with clean,
parameterizable settings. It is designed for multi-UAV setups and CI-friendly,
reproducible configurations.

> This README assumes:
> - You have a ROS 2 distro installed (e.g., Humble/ Iron).
> - `perahive-core` is checked out at: `/perahive/ws/src/perahive-core`
> - `perahive_mavros` lives inside that core repo and contains the launch files described below.

---

## 📦 What’s included

- `launch/esp-now.launch.py` — Launches **two** MAVROS nodes, fully parameterizable via launch arguments.
- Example bindings for **serial FCU** (Pixhawk/Ardupilot/PX4) and **UDP GCS** endpoints.
- Sensible defaults that make the Raspberry Pi/LAN IP **irrelevant** (binds `gcs_url` to `0.0.0.0` by default).

---

## 🧰 Prerequisites

Install MAVROS and extras for your ROS 2 distro:

```bash
sudo apt-get update
sudo apt-get install ros-${ROS_DISTRO}-mavros ros-${ROS_DISTRO}-mavros-extras
```

Serial permissions (for `/dev/ttyACM0`/`/dev/ttyUSB0`):

```bash
sudo usermod -a -G dialout $USER
# log out and back in after this change
```

---

## 📁 Package layout (typical)

```
perahive-core/
└─ perahive_mavros/
   ├─ package.xml
   ├─ setup.py
   ├─ setup.cfg
   └─ launch/
      └─ esp-now.launch.py
```

Ensure `setup.py` installs the launch files (critical for `ros2 launch` to find them):

```python
# perahive_mavros/setup.py
from setuptools import setup, find_packages
from glob import glob
import os

package_name = 'perahive_mavros'
setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Your Name',
    maintainer_email='you@example.com',
    description='Parameterized multi-UAV MAVROS launcher for Perahive',
    license='Apache-2.0',
    entry_points={'console_scripts': []},
)
```

`setup.cfg` (optional but recommended to include launch files as package data too):
```ini
[options.package_data]
* = launch/*.py
```

---

## 🛠 Build & Source

From your workspace root:

```bash
cd ~/perahive/ws
colcon build --packages-select perahive_mavros
source install/setup.bash
```

---

## 🚀 Launching

### Default (bind on all interfaces, two UAVs)

```bash
ros2 launch perahive_mavros esp-now.launch.py
```

This starts:
- `mavros` in namespace **`/uav1`** on UDP **14550**
- `mavros` in namespace **`/uav2`** on UDP **14551**
- Both use `fcu_url=serial:///dev/ttyACM0:57600` by default
- Both bind `gcs_url` to **`udp://@0.0.0.0:<port>`** so *any* GCS on the LAN can connect.

### If GCS runs on the same machine (loopback)

```bash
ros2 launch perahive_mavros esp-now.launch.py gcs_ip:=127.0.0.1
```

### If FCU is on `/dev/ttyUSB0` at 921600 baud

```bash
ros2 launch perahive_mavros esp-now.launch.py   fcu_device:=/dev/ttyUSB0 baud:=921600
```

---

## 🔧 Parameters (Launch Arguments)

| Argument | Default | Description |
|---|---|---|
| `fcu_device` | `/dev/ttyACM0` | Serial device path for the flight controller. |
| `baud` | `57600` | Serial baud rate. |
| `gcs_ip` | `0.0.0.0` | Bind address for MAVLink UDP. Use `0.0.0.0` to accept from any interface; `127.0.0.1` if GCS runs locally. |
| `uav1_ns` | `uav1` | ROS namespace for UAV 1. |
| `uav2_ns` | `uav2` | ROS namespace for UAV 2. |
| `uav1_port` | `14550` | UDP port for UAV 1. |
| `uav2_port` | `14551` | UDP port for UAV 2. |
| `system_id` | `255` | MAVLink system ID (common to both nodes by default). |
| `uav1_target_system_id` | `1` | Target system ID for UAV 1. |
| `uav2_target_system_id` | `2` | Target system ID for UAV 2. |

These arguments compose the actual MAVROS parameters:

- `fcu_url = serial:///<fcu_device>:<baud>`  
- `gcs_url = udp://@<gcs_ip>:<uavX_port>`

Examples:
- `udp://@0.0.0.0:14550` → Bind on *all* interfaces, any GCS on LAN can reach you.
- `udp://@127.0.0.1:14550` → Bind on loopback only (GCS must run on the same machine).

---

## 🔁 Additional examples (changing ports & more)

### Change only the GCS ports
Useful when running multiple UAVs or to avoid conflicts with other services.

```bash
ros2 launch perahive_mavros esp-now.launch.py   uav1_port:=14650 uav2_port:=14651
```

### Custom namespaces and system IDs
```bash
ros2 launch perahive_mavros esp-now.launch.py   uav1_ns:=drone_a uav2_ns:=drone_b   system_id:=250 uav1_target_system_id:=10 uav2_target_system_id:=20
```

### Loopback for local GCS + custom serial device
```bash
ros2 launch perahive_mavros esp-now.launch.py   gcs_ip:=127.0.0.1 fcu_device:=/dev/ttyUSB0 baud:=921600
```

### Bind to a specific NIC/IP (e.g., wlan0 has 192.168.10.42)
```bash
ros2 launch perahive_mavros esp-now.launch.py   gcs_ip:=192.168.10.42
```

> **Tip:** If you bind to a specific IP, make sure the interface is up and has that address.

---

## 🧪 Verifying MAVLink traffic

On your GCS machine, ensure it’s set to connect to the correct target (the RPi’s IP) and port:
- UAV1: `<pi-ip>:14550`
- UAV2: `<pi-ip>:14551`

You can also observe UDP packets locally:
```bash
sudo tcpdump -i any udp port 14550 or udp port 14551
```

---

## ❗ Troubleshooting

- **No connection shown in GCS:**  
  - Confirm serial device path and permissions (`dialout` group).  
  - Check that the RPi firewall allows the chosen ports.  
  - Ensure the GCS is sending to the correct host/port (Pi IP + 14550/14551).

- **Port already in use:**  
  - Choose different `uav1_port`/`uav2_port` values.  
  - Find the conflicting process: `sudo lsof -iUDP -P | grep 14550`

- **Changed launch args but nothing happens:**  
  - Restart the launch after changing args.  
  - If parameters don’t seem to apply, ensure you’re launching the correct file/package and that `setup.py` installed the launch files (rebuild if necessary).

---

## 📄 License

`perahive_mavros` is released under the **Apache-2.0** license (see `LICENSE`).

---

## 🙋 Support

If you need help tailoring the launchers (e.g., separate FCU per UAV or more than two MAVROS instances),
open an issue or ask the team. We can generate additional launch files or extend the current one.