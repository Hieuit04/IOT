# 🌱 Hệ Thống Tưới Cây Thông Minh IoT

Hệ thống tưới cây tự động sử dụng **ESP32** kết hợp các cảm biến môi trường, giao tiếp qua **MQTT** và hiển thị dữ liệu trên nền tảng **ThingsBoard Cloud**.

---

## 📋 Mục Lục

- [Mô Tả Dự Án](#-mô-tả-dự-án)
- [Kiến Trúc Hệ Thống](#-kiến-trúc-hệ-thống)
- [Phần Cứng Cần Thiết](#-phần-cứng-cần-thiết)
- [Sơ Đồ Kết Nối (Wiring)](#-sơ-đồ-kết-nối-wiring)
- [Cài Đặt Phần Mềm](#-cài-đặt-phần-mềm)
- [Cấu Hình Dự Án](#-cấu-hình-dự-án)
- [Build & Nạp Code](#-build--nạp-code)
- [Hoạt Động Của Hệ Thống](#-hoạt-động-của-hệ-thống)
- [MQTT Topics](#-mqtt-topics)
- [Cấu Trúc Thư Mục](#-cấu-trúc-thư-mục)

---

## 📖 Mô Tả Dự Án

Dự án gồm **2 module ESP32** hoạt động độc lập, cùng gửi dữ liệu lên **ThingsBoard Cloud** qua giao thức MQTT:

| Module | Chức năng |
|---|---|
| **pump_controller** | Điều khiển máy bơm tưới cây tự động/thủ công, đọc cảm biến độ ẩm đất, nhiệt độ, độ ẩm không khí (DHT11). Hỗ trợ 2 chế độ: **Auto** (tự tưới khi đất khô) và **Manual** (bật/tắt bơm bằng nút nhấn hoặc RPC từ ThingsBoard). |
| **rain** | Đọc cảm biến mưa (rain sensor) và gửi trạng thái mưa lên ThingsBoard mỗi 5 giây. Dữ liệu mưa được module pump_controller nhận và sử dụng để quyết định có tưới hay không. |

### ✨ Tính Năng Chính

- ✅ Tưới cây tự động dựa trên ngưỡng độ ẩm đất (50% – 75%)
- ✅ Tự động tắt bơm khi trời mưa vừa/to (rain level ≥ 2)
- ✅ Điều khiển bơm từ xa qua ThingsBoard RPC
- ✅ Chuyển đổi chế độ Auto / Manual qua nút nhấn hoặc RPC
- ✅ Gửi telemetry (nhiệt độ, độ ẩm, độ ẩm đất, trạng thái bơm, mưa) lên ThingsBoard
- ✅ Gửi **Cycle Summary** sau mỗi chu kỳ tưới (thời gian, lượng ẩm tăng)
- ✅ Điều khiển bơm bằng PWM (tốc độ có thể điều chỉnh)
- ✅ Buzzer thông báo sự kiện
- ✅ LED hiển thị trạng thái WiFi và chế độ hoạt động

---

## 🏗 Kiến Trúc Hệ Thống

```
┌──────────────────┐         MQTT          ┌─────────────────────┐
│   ESP32 #1       │ ───────────────────▶  │                     │
│  (Rain Sensor)   │   rain telemetry      │   ThingsBoard       │
│                  │                       │   Cloud (EU)        │
└──────────────────┘                       │                     │
                                           │  - Dashboard        │
┌──────────────────┐         MQTT          │  - RPC Control      │
│   ESP32 #2       │ ◀──────────────────▶  │  - Shared Attrs     │
│ (Pump Controller)│   telemetry + RPC     │                     │
│  - Soil Sensor   │                       └─────────────────────┘
│  - DHT11         │
│  - Pump (PWM)    │
│  - Buzzer        │
│  - Buttons       │
└──────────────────┘
```

---

## 🔧 Phần Cứng Cần Thiết

### Module Pump Controller (ESP32 #2)

| Linh kiện | Số lượng |
|---|---|
| ESP32 DOIT DevKit V1 | 1 |
| Cảm biến độ ẩm đất (Capacitive Soil Moisture) | 1 |
| Cảm biến DHT11 (nhiệt độ & độ ẩm) | 1 |
| Relay module / MOSFET driver | 1 |
| Máy bơm nước mini DC | 1 |
| Nút nhấn (button) | 2 |
| Buzzer | 1 |
| LED (WiFi indicator + Mode indicator) | 2 |
| Điện trở, dây nối, breadboard | - |

### Module Rain Sensor (ESP32 #1)

| Linh kiện | Số lượng |
|---|---|
| ESP32 DOIT DevKit V1 | 1 |
| Module cảm biến mưa (Rain Sensor) | 1 |
| Dây nối | - |

---

## 🔌 Sơ Đồ Kết Nối (Wiring)

### Pump Controller

| Chân ESP32 | Kết nối với |
|---|---|
| GPIO 34 | Cảm biến độ ẩm đất (Analog Out) |
| GPIO 14 | DHT11 (Data) |
| GPIO 25 | Máy bơm / Relay (qua MOSFET/Driver) |
| GPIO 2 | LED WiFi |
| GPIO 15 | LED Mode (Auto/Manual) |
| GPIO 32 | Nút nhấn bật/tắt bơm (INPUT_PULLUP) |
| GPIO 33 | Nút nhấn chuyển chế độ (INPUT_PULLUP) |
| GPIO 26 | Buzzer |

### Rain Sensor

| Chân ESP32 | Kết nối với |
|---|---|
| GPIO 34 | Rain Sensor (Analog Out - AO) |

---

## 💻 Cài Đặt Phần Mềm

### Yêu Cầu

- [PlatformIO IDE](https://platformio.org/install/ide) (khuyến nghị dùng extension trên VS Code)
- Hoặc [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation.html)
- Driver USB-UART cho ESP32 (CP2102 hoặc CH340 tùy board)

### Cài Đặt PlatformIO trên VS Code

1. Mở **VS Code**
2. Vào **Extensions** (Ctrl+Shift+X)
3. Tìm và cài **PlatformIO IDE**
4. Khởi động lại VS Code

---

## ⚙ Cấu Hình Dự Án

### 1. Cấu hình WiFi

Mở file source tương ứng và thay đổi thông tin WiFi:

```cpp
const char *ssid     = "Tên_WiFi_của_bạn";
const char *password = "Mật_khẩu_WiFi";
```

- **pump_controller**: sửa trong `pump_controller/src/pump_controller.cpp` (dòng 9-10)
- **rain**: sửa trong `rain/src/rain.cpp` (dòng 6-7)

### 2. Cấu hình ThingsBoard

```cpp
const char *TB_HOST  = "eu.thingsboard.cloud";   // Server ThingsBoard
const int   TB_PORT  = 1883;                       // MQTT port
const char *TB_TOKEN = "YOUR_DEVICE_TOKEN";        // Token thiết bị
```

> ⚠️ **Lưu ý**: Mỗi module ESP32 cần có **Device Token** riêng trên ThingsBoard. Hiện tại cả 2 module đang dùng chung 1 token — bạn nên tạo 2 device riêng biệt trên ThingsBoard.

### 3. Cấu hình cổng COM

Trong file `platformio.ini`, thay đổi `upload_port` cho phù hợp với cổng COM trên máy bạn:

```ini
upload_port = COM5   ; Thay bằng cổng COM thực tế
```

### 4. Điều chỉnh ngưỡng tưới (tùy chọn)

Trong `pump_controller.cpp`:

```cpp
int wetSoilVal    = 1200;  // Giá trị ADC khi đất ướt
int drySoilVal    = 3389;  // Giá trị ADC khi đất khô
int moistPerLow   = 50;    // Ngưỡng dưới (%) — bắt đầu tưới
int moistPerHigh  = 75;    // Ngưỡng trên (%) — dừng tưới
```

---

## 🚀 Build & Nạp Code

### Sử dụng PlatformIO CLI

```bash
# === Module Rain Sensor ===
cd rain
pio run                    # Build
pio run --target upload    # Nạp code vào ESP32
pio device monitor         # Mở Serial Monitor (115200 baud)

# === Module Pump Controller ===
cd pump_controller
pio run                    # Build
pio run --target upload    # Nạp code vào ESP32
pio device monitor         # Mở Serial Monitor (115200 baud)
```

### Sử dụng VS Code + PlatformIO IDE

1. Mở thư mục `pump_controller` hoặc `rain` trong VS Code
2. Nhấn nút **Build** (✓) ở thanh status bar phía dưới
3. Nhấn nút **Upload** (→) để nạp code
4. Nhấn nút **Serial Monitor** (🔌) để xem log

> ⚠️ **Lưu ý**: Mỗi lần chỉ mở **một** thư mục project (pump_controller hoặc rain) trong VS Code để PlatformIO nhận đúng file `platformio.ini`.

---

## 🔄 Hoạt Động Của Hệ Thống

### Chế Độ Auto (Mặc định)

```
Đọc độ ẩm đất
    │
    ├── Ẩm ≥ 75%  ──▶  Tắt bơm, gửi Cycle Summary
    │
    ├── Ẩm < 50% & không mưa to  ──▶  Bật bơm (chu kỳ: bơm 5s → nghỉ 10s)
    │                                      │
    │                                      ├── Sau 5s nghỉ: gửi telemetry giữa kỳ
    │                                      └── Sau 10s nghỉ: kiểm tra lại ẩm
    │
    └── Mưa vừa/to (rain ≥ 2)  ──▶  Tắt bơm ngay lập tức
```

### Chế Độ Manual

- Bật/tắt bơm bằng **nút nhấn vật lý** (GPIO 32)
- Hoặc điều khiển qua **RPC** từ ThingsBoard Dashboard

### Chu Kỳ Tưới (Watering Cycle)

Mỗi chu kỳ tưới ghi lại:
- `cycleId`: Số thứ tự chu kỳ
- `duration_s`: Thời gian tưới (giây)
- `startMoisture` / `endMoisture`: Độ ẩm trước/sau tưới
- `moistureGain`: Lượng ẩm tăng thêm (%)

---

## 📡 MQTT Topics

### Telemetry (ESP32 → ThingsBoard)

| Topic | Dữ liệu |
|---|---|
| `v1/devices/me/telemetry` | `temperature`, `humidity`, `soilMoisture`, `mode`, `pump`, `rain` |
| `v1/devices/me/telemetry` | Cycle Summary: `cycleId`, `duration_s`, `startMoisture`, `endMoisture`, `moistureGain` |

### Attributes

| Topic | Dữ liệu |
|---|---|
| `v1/devices/me/attributes` | `mode` (A/M) |

### RPC (ThingsBoard → ESP32)

| Topic | Lệnh | Mô tả |
|---|---|---|
| `v1/devices/me/rpc/request/+` | `setMode` | Chuyển chế độ Auto (`true`) / Manual (`false`) |
| `v1/devices/me/rpc/request/+` | `setWater` | Bật (`true`) / Tắt (`false`) máy bơm |

---

## 📁 Cấu Trúc Thư Mục

```
IOT/
├── pump_controller/              # Module điều khiển bơm
│   ├── platformio.ini            # Cấu hình PlatformIO (ESP32, thư viện)
│   ├── src/
│   │   └── pump_controller.cpp   # Code chính — điều khiển bơm, cảm biến, MQTT
│   ├── include/                  # Header files (nếu cần)
│   ├── lib/                      # Thư viện riêng (nếu cần)
│   └── test/                     # Unit test (nếu cần)
│
├── rain/                         # Module cảm biến mưa
│   ├── platformio.ini            # Cấu hình PlatformIO (ESP32, thư viện)
│   ├── src/
│   │   └── rain.cpp              # Code chính — đọc cảm biến mưa, gửi MQTT
│   ├── include/
│   ├── lib/
│   └── test/
│
└── README.md                     # File hướng dẫn này
```

---

## 📚 Thư Viện Sử Dụng

### Pump Controller

| Thư viện | Phiên bản | Mục đích |
|---|---|---|
| DHT sensor library (Adafruit) | ^1.4.6 | Đọc cảm biến DHT11 |
| Adafruit Unified Sensor | ^1.1.15 | Dependency cho DHT |
| BH1750 (claws) | ^1.3.0 | Cảm biến ánh sáng (dự phòng) |
| PubSubClient (knolleary) | latest | MQTT client |
| ArduinoJson (bblanchon) | ^6.21.2 | Parse JSON (dự phòng) |
| Adafruit SSD1306 | ^2.5.15 | OLED display (dự phòng) |
| Blynk | ^1.3.2 | Blynk platform (dự phòng) |
| AceButton (bxparks) | ^1.10.1 | Xử lý nút nhấn debounce |

### Rain Sensor

| Thư viện | Mục đích |
|---|---|
| PubSubClient | MQTT client |
| WiFi | Kết nối WiFi (built-in ESP32) |

---

## 🛠 Troubleshooting

| Vấn đề | Giải pháp |
|---|---|
| Upload failed — COM port not found | Kiểm tra driver USB-UART, thay đổi `upload_port` trong `platformio.ini` |
| WiFi không kết nối | Kiểm tra tên WiFi và mật khẩu trong code. Đảm bảo WiFi là 2.4GHz |
| ThingsBoard không nhận dữ liệu | Kiểm tra Device Token. Đảm bảo device đã được tạo trên ThingsBoard |
| Bơm không hoạt động | Kiểm tra kết nối relay/MOSFET. Kiểm tra nguồn cho bơm |
| Giá trị độ ẩm không chính xác | Hiệu chỉnh `wetSoilVal` và `drySoilVal` theo cảm biến thực tế |

---

## 📄 License

Dự án được phát triển cho mục đích học tập và nghiên cứu.

---

> **Tác giả**: Hieuit04  
> **Nền tảng IoT**: [ThingsBoard Cloud EU](https://eu.thingsboard.cloud)  
> **Framework**: Arduino (ESP32) + PlatformIO
