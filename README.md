🇺🇸 Read this in English | 🇧🇷 [Leia em Português](#-assistente-de-cuidado-iot-sensor-de-queda-vestível)

---

# 🚨 IoT Care Assistant: Wearable Fall Sensor

This repository contains the C++ firmware developed for a wearable device prototype focused on the safety and monitoring of the elderly. The project was built using an ESP32 microcontroller and an MPU-6050 accelerometer, aiming to detect falls, send remote alerts, and evaluate the user's physical inactivity level, all while preserving privacy (without the use of cameras).

## ⚙️ Core Features

*   **Fall Detection (Fuzzy Logic):** Analysis of Euclidean magnitude and detection of free fall followed by impact. It uses Fuzzy Logic (Sugeno Model) to calculate the fall probability and mitigate false positives.
*   **Audible Alert & Local Cancellation:** Emits a local beep (Buzzer) with a 10-second window for the user to cancel the alarm via a touch sensor, preventing false alerts from being sent to the cloud.
*   **SOS (Panic) Button:** Manual emergency trigger available to the user at any time.
*   **Telemetry via Telegram (Long Polling):** Bidirectional communication. The system sends automatic alerts and responds to manual commands (such as `/status`) via the Telegram API.
*   **Local Web Dashboard:** Asynchronous server hosted directly on the ESP32, exposing a JSON route updated via `fetch()` on the front-end for real-time metrics visualization.
*   **Inactivity Monitoring:** Finite State Machine (FSM) that evaluates uninterrupted rest time and issues progressive alerts (Mild, Moderate, Severe).

## 🛠️ Hardware Used

*   **ESP32:** Main processing and native Wi-Fi communication.
*   **MPU-6050:** Inertial sensor (6-Axis Accelerometer and Gyroscope).
*   **TTP223B:** Capacitive Touch Sensor for user interaction.
*   **Buzzer:** Local audible alert module.

## 🏗️ Software Architecture

The firmware was coded in **C++** (Arduino IDE) and designed to be entirely asynchronous. The use of blocking functions (such as `delay()`) was replaced by logic based on `millis()`, allowing the ESP32 to execute in parallel:
1. Inertial reading at 100Hz.
2. Local Web server hosting.
3. Communication with the Telegram API (Long Polling).
4. Tactile monitoring of the SOS button (with debounce lock).

---

## 🤖 AI Usage in Development

This project was developed as an academic prototype with strict deadlines. To ensure the timely delivery of the software components, I used generative AI (vibe coding) to accelerate writing the base C++ code structure.

**My main engineering role in this code consisted of:**
*   Defining the system's overall architecture (asynchronous loop, inactivity FSM, and the mathematical implementation of Fuzzy Logic).
*   Critically reviewing the generated logic and debugging integration errors.
*   Bridging logical software concepts with physical hardware behavior (MPU-6050 calibration and ESP32 pinout configuration).

As the raw coding was AI-assisted, the focus of this repository is to demonstrate the **architecture**, **product vision**, and the **proof of concept** of the integrated IoT operation, rather than micro-level optimizations in every line of code.

---

## 👥 Development Team

Project developed for the Engineering Challenges Solutions course (UFABC) by a multidisciplinary team (Computer Science, Aerospace Engineering, and Biomedical Engineering):
*   Pedro A. S. Lopes (Firmware & IoT Development)
*   Marcelo R. Ahagon
*   Felipe A. R. Caramante
*   Guilherme A. M. Almendra
