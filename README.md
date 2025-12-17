# 📘 ESP8266 GPIO Automation Firmware

**Secure, deterministic REST firmware for ESP8266-based devices**

This firmware provides a lightweight, deterministic, and secure automation platform for ESP8266-based boards
(ESP-01, ESP-01S, ESP-12, NodeMCU, Wemos D1 Mini, etc.).

It exposes a REST API to configure GPIOs, manage scheduled tasks, retrieve device status, and perform device setup, while remaining suitable for **low-memory embedded environments**.

The firmware is designed to be:

- predictable
- replay-safe
- TLS-free
- easy to integrate with custom clients and automation systems

---

# ⚙️ Key Features

## ✔ REST API Server

- JSON-based HTTP API
- Stateless request model
- Designed for embedded automation clients
- Deterministic behavior (bounded memory & execution)

---

## ✔ Secure Authentication (HMAC + Nonce)

- Optional authentication layer
- Challenge–response mechanism
- HMAC-SHA256 signatures
- One-time nonces (anti-replay)
- No passwords transmitted
- No TLS required

Authentication can be **enabled or disabled at runtime** using a dedicated setup API.

---

## ✔ GPIO Management

Supported pin modes:

- Disabled
- Digital Input
- Input with Pull-up
- Digital Output
- PWM Output
- Analog Input (`A0`)

Pin capabilities and safety constraints are enforced at runtime.

---

## ✔ Persistent Configuration

- GPIO configuration stored in **LittleFS**
- WiFi credentials stored in **EEPROM**
- Authentication key and system flags stored in **EEPROM**
- Automatic restore on reboot

---

## ✔ WiFi Captive Portal

- If no WiFi credentials are present, the device starts in AP mode
- A configuration page is served via embedded web portal
- Used for initial provisioning or recovery

---

## ✔ Cron Scheduler

- 32 persistent cron job slots
- Standard 5-field cron syntax
- Supported actions:

  - Set GPIO state
  - Toggle GPIO
  - Set PWM value
  - Device reboot

- Cron jobs are included in `/api/state`

---

# 🔐 Authentication & Security Model

## Overview

The firmware implements a **nonce-based HMAC authentication mechanism** tailored for embedded devices.

### Design Goals

- Avoid TLS overhead
- Prevent credential leakage
- Prevent replay attacks
- Keep RAM usage predictable
- No dynamic allocation during auth verification

---

## Authentication Flow

### 1️⃣ Request a challenge (public endpoint)

```
GET /api/auth/challenge
```

This endpoint is **never authenticated**.

### Response

```json
{
  "nonce": 1933383571
}
```

Nonce properties:

- Generated using a secure random source
- Bound to the client IP
- Has a limited lifetime
- Can be used **only once**

---

### 2️⃣ Compute the signature

For every protected request, the client computes:

```
HMAC_SHA256(
  nonce + uri + payload,
  AUTH_SECRET
)
```

#### Signature Components

| Component     | Description                             |
| ------------- | --------------------------------------- |
| `nonce`       | Value returned by `/api/auth/challenge` |
| `uri`         | Request path (e.g. `/api/state`)        |
| `payload`     | Raw request body (empty for GET)        |
| `AUTH_SECRET` | Shared secret stored in EEPROM          |

⚠️ The string must match **byte-for-byte**.

---

### 3️⃣ Send authenticated request

Required headers:

| Header    | Description             |
| --------- | ----------------------- |
| `X-Nonce` | Nonce value             |
| `X-Auth`  | Hex-encoded HMAC-SHA256 |

---

## Payload Rules (CRITICAL)

- **GET requests** → payload is an empty string
- **POST / PATCH requests** → payload is the raw JSON body
- Any whitespace or JSON reformatting breaks the signature

---

## Authentication Errors

```json
{ "error": "unauthorized" }
```

Returned on:

- Missing headers
- Invalid nonce
- Expired nonce
- Signature mismatch
- Replay attempt

HTTP status: **401**

---

# 🔧 Device Setup & Provisioning API

This API is used to configure **system-level settings**.

It is intended for:

- first installation
- recovery
- enabling authentication
- toggling serial debug output

---

## Access Rules

| Authentication state | Setup API access        |
| -------------------- | ----------------------- |
| Auth disabled        | Public                  |
| Auth enabled         | Requires authentication |

---

## POST /api/setup

Configures system flags and optionally enables authentication.

### Request Body

```json
{
  "auth": true,
  "serialDebug": false
}
```

### Fields

| Field         | Type | Description                           |
| ------------- | ---- | ------------------------------------- |
| `auth`        | bool | Enable or disable API authentication  |
| `serialDebug` | bool | Enable or disable serial debug output |

---

### Behavior

#### Authentication flag

- If `auth` is set to `true` **and authentication was disabled**:

  - A new random authentication key is generated
  - The key is stored in EEPROM
  - Authentication is enabled
  - The key is returned **once** in the response

- If `auth` is set to `false`:

  - Authentication is disabled
  - Stored key is erased

#### Serial debug flag

- Stored persistently
- Affects runtime logging behavior

---

### Example Response (auth enabled)

```json
{
  "auth": true,
  "serialDebug": false,
  "authKey": "7f9c4e2d8a0b1c6e9d4a3f8b2c7e1d9a"
}
```

⚠️ The authentication key is returned **only once**.
If lost, authentication must be disabled and re-enabled.

---

### Example Response (auth disabled)

```json
{
  "auth": false,
  "serialDebug": true
}
```

---

# 📡 REST API

Base URL:

```
http://<device-ip>/api
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
    "serialDebug": true,
    "auth": true,
    "uptime": 114
  },
  "cronJobs": {
    "0": {
      "state": "Active",
      "cron": "30 18 * * *",
      "action": "set",
      "pin": "GPIO4",
      "value": 1
    }
  },
  "pins": {
    "GPIO4": {
      "mode": "Output",
      "state": 1,
      "capabilities": ["Input", "InputPullup", "Output", "Pwm"],
      "safety": "Safe"
    }
  }
}
```

---

## GET /api/pin?id=GPIOx 🔐

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

## POST /api/config 🔐

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

## PATCH /api/pin/set 🔐

Updates pin mode and/or state.

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

# 🕒 Cron Scheduler API 🔐

## PATCH /api/cron/set

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

---

## 6.2 GET /api/cron?id=5

Lists all 32 scheduled jobs.

```json
{
  "state": "Active",
  "cron": "*/5 * * * *",
  "action": "pwm",
  "pin": "GPIO5",
  "value": 128
}
```

---

## 6.3 DELTE /api/cron?id=5

Disactive a job by ID.

```json
{ "id": 5 }
```

---

## DELETE /api/cron/clear

Removes all jobs.

```json
{ "success": true }
```

---

# 🛑 Error Handling

| Condition         | HTTP | Response                           |
| ----------------- | ---- | ---------------------------------- |
| Missing parameter | 400  | `{ "error": "missing parameter" }` |
| Invalid value     | 400  | `{ "error": "invalid value" }`     |
| Unauthorized      | 401  | `{ "error": "unauthorized" }`      |
| Internal error    | 500  | `{ "success": false }`             |

---

# 🔒 Security Notes

- One nonce = one request
- Nonces are IP-bound
- Nonces are invalidated immediately
- Constant-time HMAC comparison
- No heap allocation during auth verification
- EEPROM initialized using a magic value

---

# 🧩 Project Structure

```
lib/
  ApiManager/
  Auth/
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
