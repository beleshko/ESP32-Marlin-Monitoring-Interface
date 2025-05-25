# ESP32 3D Printer Monitoring Dashboard

![Project Banner](assets/preview.png) <!-- Add actual preview image -->

A Material Design-inspired monitoring system for 3D printers, featuring real-time visualization and network connectivity. Built for ESP32 with 240x240 RGB display support.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-00979D.svg)](https://www.espressif.com/)
[![Library: ESP_8_BIT_GFX](https://img.shields.io/badge/Library-ESP__8__BIT__GFX-blue.svg)](https://github.com/Roger-random/ESP_8_BIT_GFX)

## 🌟 Key Features

| Category        | Details                                                                 |
|-----------------|-------------------------------------------------------------------------|
| **Visual Design** | Material Dark Theme • Animated transitions • Custom icon set • Responsive layout |
| **Monitoring**  | Dual temperature tracking • Historical graph • Print progress bar • System status indicators |
| **Connectivity**| WiFi Manager • NTP Clock • Async TCP Server • Serial communication      |
| **Optimization**| 240MHz CPU mode • Frame buffer management • Smoothing algorithms        |

## 🛠 Hardware Setup

```plaintext
┌──────────────┐        UART2         ┌───────────────┐
│   ESP32      │<──(GPIO16/17)───────>│ Arduino MEGA  │
│              │                      │               │
│  Display     │                      └───────────────┘
│  Interface   │                             ▲
└──────┬───────┘                             │
       │                             3D Printer
       │ 240x240 RGB                     Control
       ▼  Display                          Lines
┌──────────────┐
│  8-Bit       │
│  RGB Screen  │
└──────────────┘

📥 Installation
Dependencies
bash

# PlatformIO
pio lib install "ESP_8_BIT_GFX" "AsyncTCP"

# Arduino IDE
Tools > Manage Libraries > Search:
- ESP_8_BIT_GFX
- AsyncTCP

Configuration

    Set WiFi credentials:

cpp

const char* ssid = "YOUR_NETWORK";
const char* password = "YOUR_PASSWORD";

    Adjust display settings (if needed):

cpp

const uint16_t SCREEN_W = 240;  // X resolution
const uint16_t SCREEN_H = 240;  // Y resolution

    Configure NTP:

cpp

configTime(3*3600, 0, "pool.ntp.org");  // UTC+3

📡 Data Protocol

MEGA Controller should send serial data formatted as:
plaintext

T:[Current Extruder] /[Target] B:[Current Bed] /[Target]
M73 P[Progress Percent]

Example:
plaintext

T:215.4 /220 B:65.2 /70
M73 P42.8

🎨 Interface Components
Element	Description	Color Code
Status Bar	IP, Time, Connection Indicators	#242424
Temperature Cards	Extruder/Bed with progress bars	#FF9800
History Graph	120-point rolling temperature display	#FFC107
Progress Panel	Percentage indicator with custom icon	#2196F3
⚙️ Performance Tips

    Enable maximum CPU frequency:

cpp

setCpuFrequencyMhz(240);

    Disable Bluetooth:

cpp

btStop();

    Use hardware-accelerated graphics:

cpp

videoOut.waitForFrame();  // Maintains 15-20 FPS

🤝 Contributing

    Fork the repository

    Create feature branch (git checkout -b feature/AmazingFeature)

    Commit changes (git commit -m 'Add some AmazingFeature')

    Push to branch (git push origin feature/AmazingFeature)

    Open Pull Request

📜 License

Distributed under MIT License. See LICENSE for more information.

Made with ❤️ by [Your Name] | *Optimized for 8-bit retro aesthetics* 🕹️
