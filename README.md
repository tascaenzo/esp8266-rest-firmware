# 📘 ESP8266 GPIO Automation Firmware

**REST-enabled firmware for ESP8266-based devices**

This project provides a feature-rich firmware for the ESP8266 (ESP-01, ESP-01S, ESP-12, NodeMCU, Wemos D1 Mini…) including:

- A fully configurable **GPIO controller**
- A simple **REST API** for home automation systems
- A **WiFi configuration portal**
- Persistent storage of configurations
- Automatic boot-time restoration via flash memory

It is engineered to be lightweight, reliable, and modular.

---

# ⚙️ Features

### ✔️ REST API server

Expose GPIO states, pin metadata and device info through a JSON API.

### ✔️ Full GPIO configuration

Supports:

- Digital input
- Input with pull-up
- Digital output
- PWM output
- Analog input (`A0`)

### ✔️ Auto-save configuration

GPIO configurations persist using LittleFS.
WiFi credentials are stored in EEPROM.

### ✔️ Captive WiFi portal

If no saved WiFi credentials exist, the ESP starts an AP and presents a setup page.

### ✔️ Centralized hardware abstraction

`GpioConfig` and `DeviceController` provide a stable GPIO backend.

### ✔️ Cron Scheduler (NEW)

The firmware includes a 32-slot Cron Scheduler:

- Supports standard 5-field cron expressions
- Executes GPIO set/pwm actions or system reboot
- Jobs are stored persistently
- Jobs are automatically listed inside GET /api/state

---

# 🧱 Data Model

Each GPIO is represented internally using:

```cpp
struct GpioConfig {
  uint8_t pin;     // GPIO number (0–16 or A0)
  PinMode mode;    // Disabled, Input, InputPullup, Output, Pwm, Analog
  int state;       // Last known logic level or analog/PWM value
};
```

Cron Job Structure:

```cpp
struct CronJob {
  bool active;       // Job enabled/disabled
  char cron[32];     // "min hour day month weekday"
  char action[16];   // "set", "pwm", "reboot"
  uint8_t pin;       // Target pin (if applicable)
  int value;         // State/PWM value (if applicable)
};
```

This structure maps directly to the REST API.

---

# 📡 REST API Documentation

API root:

```
http://<device-ip>/api/...
```

All responses are JSON.

---

# 1. GET /api/state

Returns complete device info and the full GPIO table.

### 🔍 Response Model

Each pin entry includes:

| Field          | Meaning                                                 |
| -------------- | ------------------------------------------------------- |
| `mode`         | Current pin mode                                        |
| `state`        | Current logic or analog value                           |
| `capabilities` | Supported modes for this pin                            |
| `safety`       | Safety classification (`Safe`, `Warn`, `BootSensitive`) |

### 🔐 Safety Levels

- `"Safe"` → fully safe to use as output
- `"Warn"` → limited behavior (e.g. serial pins, GPIO16)
- `"BootSensitive"` → affects boot mode (GPIO0, GPIO2, GPIO15)

### 📄 Example Response

```json
{
  "device": {
    "device": "ESP8266",
    "ip": "192.168.1.8",
    "chip": 12970503,
    "rssi": -78,
    "uptime": 114
  },
  "cron": [
    {
      "id": 0,
      "active": true,
      "cron": "30 18 * * *",
      "action": "set",
      "pin": "GPIO4",
      "value": 1
    }
  ],
  "pins": {
    "GPIO0": {
      "mode": "Disabled",
      "state": 0,
      "capabilities": ["Input", "InputPullup", "Output", "Pwm"],
      "safety": "critical"
    },
    "GPIO2": {
      "mode": "Disabled",
      "state": 0,
      "capabilities": ["Input", "InputPullup", "Output", "Pwm"],
      "safety": "critical"
    },
    "GPIO4": {
      "mode": "Disabled",
      "state": 0,
      "capabilities": ["Input", "InputPullup", "Output", "Pwm"],
      "safety": "safe"
    },
    "GPIO16": {
      "mode": "Disabled",
      "state": 0,
      "capabilities": ["Input", "Output"],
      "safety": "warn"
    },
    "A0": {
      "mode": "Analog",
      "state": 4,
      "capabilities": ["Analog"],
      "safety": "safe"
    }
  }
}
```

---

# 2. GET /api/pin?id=XX

Returns configuration and state for a single pin.

### Example Request

```
GET /api/pin?id=GPIO4
```

### Example Response

```json
{
  "id": "GPIO4",
  "gpio": 4,
  "mode": "Output",
  "state": 1
}
```

### Errors

```json
{ "error": "missing pin" }
```

```json
{ "error": "invalid pin" }
```

---

# 3. POST /api/config

Replaces the **entire GPIO configuration table**.

Payload keys correspond directly to pin IDs:

- `GPIO0` … `GPIO16`
- `A0` (analog only)

### Example Body

```json
{
  "GPIO12": { "mode": "Output", "state": 1 },
  "GPIO5": { "mode": "InputPullup" },
  "A0": { "mode": "Analog" }
}
```

Pins omitted in the JSON are automatically set to **Disabled**.

### Success Response

```json
{ "success": true }
```

---

# 4. PATCH /api/pin/set

Sets the **digital output state** of a configured Output pin.

```json
{
  "mode": "Output",
  "state": 1
}
```

---

# 5. POST /api/reboot

Restarts the ESP.

### Response

```json
{ "rebooting": true }
```

---

# 🛑 Common Error Codes

| Condition          | HTTP | JSON                               |
| ------------------ | ---- | ---------------------------------- |
| Missing parameter  | 400  | `{ "error": "missing id" }`        |
| Invalid pin        | 400  | `{ "error": "invalid pin" }`       |
| Invalid mode       | 400  | `{ "error": "invalid mode" }`      |
| PWM not supported  | 400  | `{ "error": "PWM not supported" }` |
| Save/Apply failure | 500  | `{ "success": false }`             |

---

# 🕒 Cron Scheduler API (NEW)

The scheduler supports **32 job slots** with persistent storage.

Cron syntax:

```
minute hour day month weekday
```

Examples:

- `"*/10 * * * *"` → every 10 minutes
- `"0 7 * * 1"` → every Monday at 07:00
- `"30 18 * * *"` → every day at 18:30

---

## 6.1 POST /api/cron/add

Adds a new Cron job.

### Request

```json
{
  "cron": "30 18 * * *",
  "action": "set",
  "pin": "GPIO4",
  "value": 1
}
```

### Response

```json
{ "success": true, "id": 3 }
```

---

## 6.2 GET /api/cron/list

Lists all 32 scheduled jobs.

```json
{
  "jobs": [
    {
      "id": 0,
      "active": true,
      "cron": "*/5 * * * *",
      "action": "pwm",
      "pin": "GPIO5",
      "value": 128
    },
    { "id": 1, "active": false }
  ]
}
```

---

## 6.3 POST /api/cron/delete

Deletes a job by ID.

```json
{ "id": 4 }
```

---

## 6.4 POST /api/cron/clear

Removes all jobs.

```json
{ "success": true }
```

---

# 🛑 Common Error Codes

| Condition          | HTTP | JSON                               |
| ------------------ | ---- | ---------------------------------- |
| Missing parameter  | 400  | `{ "error": "missing id" }`        |
| Invalid pin        | 400  | `{ "error": "invalid pin" }`       |
| Invalid mode       | 400  | `{ "error": "invalid mode" }`      |
| Invalid cron       | 400  | `{ "error": "invalid cron" }`      |
| PWM not supported  | 400  | `{ "error": "PWM not supported" }` |
| No free slot       | 400  | `{ "error": "no free job slot" }`  |
| Save/Apply failure | 500  | `{ "success": false }`             |

---

# 🚀 Build Instructions

```bash
pio run
```

Upload:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor
```

---

# 🔧 Flashing Notes (ESP-01 / ESP-01S)

- Pull **GPIO0 → GND** during reset to enter flash mode
- Typical baudrate: **115200**

---

# 🧩 Recommended Project Structure

```
lib/
  ApiManager/
  BinaryStorage/
  CronScheduler/
  DeviceController/
  EepromConfig/
  GpioTypes/
  GpioUtils/
  WebPortal/
  WifiManager/
src/
  main.cpp
include/
```
