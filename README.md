# ESP32 3D Printer Monitor & UART Bridge

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)
![3D Printing](https://img.shields.io/badge/3D_Printing-FF6B35?style=for-the-badge&logo=3d&logoColor=white)

**3D Printer monitoring system with composite video output and UART bridge**

[Features](#features) • [Installation](#installation) • [Usage](#usage) 

</div>

---

## 📋 Overview

ESP32 3D Printer Monitor is a comprehensive solution for 3D printer monitoring that includes:

- **Composite video output** with Material Design interface
- **UART bridge** for remote control via COM port
- **Web monitoring** of temperatures and print progress
- **Temperature history** with graphical display

## ✨ Features

### 🖥️ Video Interface
- Material Design Dark Theme
- Real-time extruder and bed temperature display
- Temperature history graph (120 data points)
- Print progress bar
- Connection status indicators (WiFi, UART Bridge, Arduino Mega)
- NTP time synchronization

### 🌐 Network Capabilities
- **TCP server** on port 4097 for monitoring
- **Telnet server** on port 23 for UART bridge
- Automatic reconnection
- Multiple client support

### 🔧 UART Bridge
- Transparent data transmission between network and UART
- Virtual COM port on PC
- Windows and Linux support
- Automatic ESP32 network discovery

## 🔧 Hardware Requirements

### ESP32
- **Model**: ESP32 DevKit or compatible
- **Flash**: minimum 4MB
- **RAM**: 520KB (standard)
- **Pin connections**:
  - GPIO 25: Composite video output
  - GPIO 16: UART RX (to Arduino Mega TX)
  - GPIO 17: UART TX (to Arduino Mega RX)

### Additional Hardware
- Arduino Mega 2560 (or compatible board)
- 470 Ohm resistor for composite video
- 100 nF capacitor (optional)
- RCA connector for video output

## 📦 Installation

### 1. Arduino IDE Setup

```bash
# Install ESP32 board package
# In Arduino IDE: File → Preferences → Additional Board Manager URLs
https://dl.espressif.com/dl/package_esp32_index.json
```

### 2. Library Installation

Install via Library Manager:
- `ESP_8_BIT_GFX` by bitluni
- `AsyncTCP` by me-no-dev
- `WiFi` (built-in)

### 3. Firmware Configuration

Open `Interface.ino` and configure:

```cpp
const char* ssid     = "Your_WiFi_Network";
const char* password = "WiFi_Password";
```

### 4. Upload Firmware

1. Connect ESP32 to computer
2. Select board: `ESP32 Dev Module`
3. Configure settings:
   - **CPU Frequency**: 240MHz
   - **Flash Size**: 4MB
   - **Partition Scheme**: Default
4. Upload firmware

### 5. Python Script Installation

```bash
# Clone repository
git clone <repository-url>
cd esp32-printer-monitor

# Install dependencies
pip install -r requirements.txt

# Or manually
pip install pyserial
```

#### Additional for Windows
For full functionality, install **com0com**:
1. Download from [official website](http://com0com.sourceforge.net/)
2. Install and create virtual port pair

## 🚀 Usage

### System Startup

1. **Connect hardware** according to wiring diagram
2. **Upload firmware** to ESP32
3. **Find ESP32 IP address** in Serial Monitor
4. **Connect video output** to monitor/TV

### Using UART Bridge

#### Automatic Discovery
```bash
python Connect.py --scan
```

#### Connect to Specific IP
```bash
# Linux
python Connect.py --ip 192.168.1.100

# Windows with port specification
python Connect.py --ip 192.168.1.100 --port COM8
```

#### All Parameters
```bash
python Connect.py --help
```

### Command Line Parameters

| Parameter | Short Form | Description | Default |
|-----------|------------|-------------|---------|
| `--ip` | `-i` | ESP32 IP address | - |
| `--port` | `-p` | COM port (Windows) | - |
| `--esp-port` | `-e` | ESP32 port | 23 |
| `--scan` | `-s` | Scan network | - |

## 🔌 Wiring Diagram

### ESP32 ↔ Arduino Mega
```
ESP32 GPIO 16 (RX) ←→ Arduino Mega Pin 1 (TX)
ESP32 GPIO 17 (TX) ←→ Arduino Mega Pin 0 (RX)
ESP32 GND         ←→ Arduino Mega GND
```

### Composite Video Output
```
ESP32 GPIO 25 → [470Ω Resistor] → RCA center pin
ESP32 GND     → RCA ground pin
```

### Connection Diagram
```
                    ┌─────────────┐
                    │   ESP32     │
                    │             │
    ┌───────────────┤ GPIO 25     │
    │               │             │
    │   470Ω        │ GPIO 16 (RX)├──── Arduino TX
    │   ┌─┴─┐       │             │
    └───┤   ├───────┤ GPIO 17 (TX)├──── Arduino RX
        └───┘       │             │
          │         │ GND         ├──── Arduino GND
    ┌─────┴─────┐   └─────────────┘
    │  RCA Jack │
    │     ○     │        ┌─────────────────┐
    │    ╱ ╲    │        │  Arduino Mega   │
    └───────────┘        │                 │
         │               │  3D Printer     │
    ┌────┴────┐          │  Controller     │
    │ Monitor │          └─────────────────┘
    └─────────┘
```

## 📺 Monitor Interface

### Main Screen
- **App Bar**: IP address, time, connection status
- **Temperature Cards**: 
  - Extruder (current/target temperature)
  - Bed (current/target temperature)
  - Temperature progress bars
- **History Graph**: extruder temperature for last 2 minutes
- **Print Progress**: task completion percentage

### Color Scheme (Material Design Dark)
| Element | Color | RGB332 |
|---------|-------|--------|
| Background | Black | 0x00 |
| Surface | Dark Gray | 0x24 |
| Primary | Blue | 0x9F |
| Secondary | Teal | 0x7C |
| Error | Red | 0xE0 |
| Success | Green | 0x1C |
| Warning | Orange | 0xFC |

## 🌐 API and Protocols

### TCP Monitoring (port 4097)
Accepts data from Arduino in format:
```
T:210.5 /210.0 B:60.2 /60.0    # Temperatures
M73 P45                         # Print progress
```

### UART Bridge (port 23)
Transparent bridge between TCP and UART:
- All client data → UART
- All UART data → client
- Multiple monitoring connections supported
- One active UART bridge client

## 🖥️ Working with Virtual COM Port

### Linux
```bash
# Start bridge
python Connect.py --ip 192.168.1.100

# Script will show port path, e.g.:
# Virtual port created: /dev/pts/2

# Using with screen
screen /dev/pts/2 115200

# Using with minicom
minicom -D /dev/pts/2 -b 115200
```

### Windows
```bash
# With com0com (recommended)
# 1. Create port pair COM8 ↔ COM9
# 2. Start bridge on COM8
python Connect.py --ip 192.168.1.100 --port COM8

# 3. Connect applications to COM9
# PuTTY, Arduino IDE, Pronterface, etc.
```

## 🔧 com0com Setup (Windows)

### Installation
1. Download com0com from official website
2. Install with administrator rights
3. Restart system

### Port Pair Configuration
```cmd
# Run command prompt as administrator
cd "C:\Program Files\com0com"

# Create port pair
setupc install PortName=COM8 PortName=COM9

# Check created ports
setupc list
```

### Usage
- **COM8**: for Connect.py (--port COM8 parameter)
- **COM9**: for your applications (PuTTY, Arduino IDE, etc.)

## 🛠️ Usage Examples

### Monitoring via Telnet
```bash
# Direct connection to ESP32
telnet 192.168.1.100 23

# Now you can send G-code commands
M105  # Request temperatures
M73 P50  # Set progress to 50%
```

### Using with OctoPrint
1. Start Connect.py
2. Configure Serial Port in OctoPrint to virtual port
3. Connect to printer through OctoPrint

### Using with Pronterface
1. Start Connect.py
2. Select virtual COM port in Pronterface
3. Set baud rate to 115200
4. Connect to printer

## 📊 Monitoring and Debugging

### ESP32 Logs
Connect to Serial Monitor (115200 baud) to view:
- WiFi connection status
- Device IP address
- TCP connection status
- Received commands

### Network Connection Check
```bash
# Check ESP32 ports
nmap -p 23,4097 192.168.1.100

# Check TCP connection
nc -v 192.168.1.100 23
```

### Virtual Port Debugging

#### Linux
```bash
# List active pts
ls -la /dev/pts/

# Monitor traffic
sudo cat /dev/pts/2
```

#### Windows
```bash
# List COM ports
mode

# Check port availability
mode COM8
```

## ⚡ Performance

### System Characteristics
- **Screen refresh rate**: ~15 FPS (66ms)
- **Temperature smoothing**: coefficient 0.15
- **Temperature history**: 120 points (2 minutes at 1 sec frequency)
- **Mega connection timeout**: 1 second
- **TCP timeout**: 5 seconds

### Optimization
- ESP32 CPU frequency: 240MHz for stable video operation
- Bluetooth disabled to save memory
- Asynchronous TCP connections for multiple clients

## 🚨 Troubleshooting

### ESP32 Won't Connect to WiFi
```cpp
// Check settings in Interface.ino
const char* ssid     = "Exact_Network_Name";
const char* password = "Exact_Password";
```

### No Video Signal
- Check 470 Ohm resistor on GPIO 25
- Verify correct RCA connection
- Try different monitor/TV

### Connect.py Can't Find ESP32
```bash
# Check ping
ping 192.168.1.100

# Check port
telnet 192.168.1.100 23

# Scan network
python Connect.py --scan
```

### Virtual Port Not Created
#### Linux
```bash
# Check access permissions
sudo python Connect.py --ip 192.168.1.100
```

#### Windows
```bash
# Install com0com
# Run as administrator
python Connect.py --ip 192.168.1.100 --port COM8
```

### Arduino Mega Not Responding
- Check UART connection (RX ↔ TX)
- Ensure common GND
- Verify 115200 baud rate on both sides
- Ensure printer firmware is loaded on Mega

## 📁 Project Structure

```
esp32-printer-monitor/
├── Interface.ino          # ESP32 firmware
├── Connect.py            # Python UART bridge script
├── README.md             # This file
├── requirements.txt      # Python dependencies
├── docs/                 # Additional documentation
│   ├── wiring.md        # Detailed wiring guide
│   └── troubleshooting.md # Extended troubleshooting
└── examples/            # Usage examples
    ├── octoprint_config.md
    └── pronterface_setup.md
```

## 🔄 Data Flow

```
[3D Printer App] ↔ [Virtual COM Port] ↔ [Connect.py] ↔ [TCP/WiFi] ↔ [ESP32] ↔ [UART] ↔ [Arduino Mega]
                                                           ↓
                                                    [Composite Video] → [Monitor/TV]
```

## 📋 Requirements File

Create `requirements.txt`:
```
pyserial>=3.5
```

## 🔐 Security Considerations

- ESP32 operates on local network only
- No authentication implemented (designed for trusted networks)
- UART bridge provides direct access to printer
- Consider network isolation for production use

## 🎯 Future Enhancements

- [ ] Web interface for remote monitoring
- [ ] Authentication and security features
- [ ] SD card logging capability
- [ ] Multiple printer support
- [ ] Mobile app integration
- [ ] Advanced temperature alerts

## 📝 License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## 🤝 Contributing

We welcome contributions to this project! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Create a Pull Request

### Development Guidelines
- Follow existing code style
- Add comments for complex functions
- Update documentation for new features
- Test on both Windows and Linux

## 🙏 Acknowledgments

- **bitluni** for the ESP_8_BIT_GFX library
- **me-no-dev** for AsyncTCP library
- **ESP32 community** for extensive documentation
- **3D printing community** for feedback and testing

## 📞 Support

If you encounter issues or have questions:

1. Check the [Troubleshooting](#troubleshooting) section
2. Create an Issue on GitHub
3. Include logs and problem description
4. Specify your hardware setup

## 📊 Compatibility

### Tested Hardware
- ✅ ESP32 DevKit v1
- ✅ ESP32 WROOM-32
- ✅ Arduino Mega 2560
- ✅ Various composite video monitors
- ✅ CRT TVs with composite input

### Tested Software
- ✅ Windows 10/11
- ✅ Ubuntu 20.04/22.04
- ✅ Raspberry Pi OS
- ✅ OctoPrint
- ✅ Pronterface
- ✅ Arduino IDE Serial Monitor

---

<div align="center">

**Made with ❤️ for the 3D printing community**

⭐ Star this project if it helped you! ⭐

</div>
