Visually impaired people often rely on passive white canes to navigate, which detect obstacles only upon contact and struggle to identify drop-offs or head-level hazards. To address this, we developed Smart Stick, an IoT attachment that enhances spatial awareness through real-time sensing and cloud connectivity. The goal of this project was to build a cost-effective, multi-sensor system that provides immediate audio feedback while enabling remote configuration and data analysis.

The system design utilizes a distributed architecture: an Arduino Uno serves as the central processing unit, reading data from two ultrasonic sensors (for peripheral obstacles) and a Time-of-Flight sensor (for detecting drop-offs/dips). This unit communicates via serial connection to an ESP32, which acts as a dual-protocol gateway. The ESP32 handles Bluetooth Low Energy (BLE) to allow users to dynamically adjust sensor sensitivity via a mobile app, and WiFi to transmit telemetry data to AWS IoT Core using MQTT.

Our results demonstrate a fully functional end-to-end IoT pipeline. The device successfully detects obstacles and dangerous drops within user-defined thresholds (e.g., 30cm for walls, 60cm for drops). The data is transmitted to the cloud and consumed by a Python-based dashboard that provides real-time visualization and text-to-speech audio alerts.


