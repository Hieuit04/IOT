#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <AceButton.h>
using namespace ace_button;

/* ---------------- WiFi ---------------- */
const char *ssid = "Anh em 37";
const char *password = "naptienroicho";

/* ---------------- ThingsBoard MQTT ---------------- */
const char *TB_HOST = "eu.thingsboard.cloud"; 
const int TB_PORT = 1883;
const char *TB_TOKEN = "JIXUjdNSo5IKcPjlKiQy";

WiFiClient espClient;
PubSubClient client(espClient);

/* ---------------- Hardware Pins ---------------- */
#define SensorPin 34
#define DHTPin 14
#define PumpPin 25
#define wifiLed 2
#define RelayButtonPin 32
#define ModeSwitchPin 33
#define BuzzerPin 26
#define ModeLed 15
#define DHTTYPE DHT11


/* ---------------- Variables ---------------- */
int wetSoilVal = 1200;
int drySoilVal = 3389;
int moistPerLow = 50;
int moistPerHigh = 75;

int sensorVal = 0;
int moisturePercentage = 0;
bool togglePump = LOW;
bool prevMode = true;
int temperature1 = 0;
int humidity1 = 0;
int rainLevel = 0;
String rainText = "No Rain";
String currMode = "A";

unsigned long cycleStartTime = 0;
int startMoisture = 0;
int endMoisture = 0;
int cycleId = 0;

DHT dht(DHTPin, DHTTYPE);
ButtonConfig config1, config2;
AceButton button1(&config1), button2(&config2);

/* ---------------- PWM Pump ---------------- */
#define PWM_CHANNEL 0
#define PWM_FREQ 1000
#define PWM_RES 8
#define PUMP_SPEED 220

/* ---------------- Function Prototypes ---------------- */
void controlBuzzer(int duration);
void button1Handler(AceButton *, uint8_t, uint8_t);
void button2Handler(AceButton *, uint8_t, uint8_t);
void getMoisture();
void getWeather();
void controlMoist();
void sendTelemetry();
void reconnect();
void applyMode(bool autoMode);
void applyPump(bool on);
void parseRainLevelFromText(const String &txt);
void sendCycleSummary();
// void randomizeMoistureThresholds();

/* ---------------- Helpers ---------------- */
void pumpSpeed(int duty) { ledcWrite(PWM_CHANNEL, duty); }

void controlBuzzer(int duration)
{
  digitalWrite(BuzzerPin, HIGH);
  delay(duration);
  digitalWrite(BuzzerPin, LOW);
}


void getMoisture()
{
  sensorVal = analogRead(SensorPin);
  if (sensorVal > (wetSoilVal - 100) && sensorVal < (drySoilVal + 100))
  {
    moisturePercentage = map(sensorVal, drySoilVal, wetSoilVal, 0, 100);
    moisturePercentage = constrain(moisturePercentage, 0, 100);
  }
}

void getWeather()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t))
  {
    humidity1 = int(h);
    temperature1 = int(t);
  }
}

void parseRainLevelFromText(const String &txt)
{
  if (txt.indexOf("Heavy") >= 0)
    rainLevel = 3;
  else if (txt.indexOf("Moderate") >= 0)
    rainLevel = 2;
  else if (txt.indexOf("Light") >= 0)
    rainLevel = 1;
  else
    rainLevel = 0;
}

/* ---------------- Telemetry ---------------- */
void sendTelemetry()
{
  getMoisture();
  getWeather();
  currMode = prevMode ? "A" : "M";

  String payload = "{";
  payload += "\"temperature\":" + String(temperature1) + ",";
  payload += "\"humidity\":" + String(humidity1) + ",";
  payload += "\"soilMoisture\":" + String(moisturePercentage) + ",";
  payload += "\"mode\":\"" + currMode + "\",";
  payload += "\"pump\":" + String(togglePump ? "On" : "Off") + ",";
  payload += "\"rain\":\"" + rainText + "\"";
  payload += "}";

  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Telemetry: " + payload);
}

void sendCycleSummary()
{
  unsigned long duration_s = (millis() - cycleStartTime) / 1000.0;
  int moistureGain = endMoisture - startMoisture;

  String payload = "{";
  payload += "\"cycleId\":" + String(cycleId) + ",";
  payload += "\"duration_s\":" + String(duration_s) + ",";
  payload += "\"startMoisture\":" + String(startMoisture) + ",";
  payload += "\"endMoisture\":" + String(endMoisture) + ",";
  payload += "\"moistureGain\":" + String(moistureGain) + ",";
  payload += "\"lowThreshold\":" + String(moistPerLow) + ",";
  payload += "\"highThreshold\":" + String(moistPerHigh);
  payload += "}";

  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Cycle Summary Sent: " + payload);
}

void applyMode(bool autoMode)
{
  prevMode = autoMode;
  currMode = prevMode ? "A" : "M";
  digitalWrite(ModeLed, prevMode);

  String payload = "{\"mode\":\"" + currMode + "\"}";
  client.publish("v1/devices/me/telemetry", payload.c_str());
  client.publish("v1/devices/me/attributes", payload.c_str());
  Serial.println("Mode sync: " + currMode);
}

void applyPump(bool on)
{
  togglePump = on;
  if (togglePump)
    pumpSpeed(PUMP_SPEED);
  else
    pumpSpeed(0);

  sendTelemetry();
  Serial.println(togglePump ? "Pump ON" : "Pump OFF");
}

/* ---------------- MQTT callback ---------------- */
void callback(char *topic, byte *payload, unsigned int length)
{
  // Nhận và chuyển đổi nội dung gói tin
  String msg;
  for (unsigned int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  String topicStr(topic);
  Serial.println(topicStr + " → " + msg);

  // Kiểm tra xem đây có phải là RPC request từ ThingsBoard không
  if (topicStr.startsWith("v1/devices/me/rpc/request/"))
  {
    // Lấy request ID (để phản hồi đúng yêu cầu RPC)
    int requestId = atoi(strrchr(topicStr.c_str(), '/') + 1);

    // RPC: Đổi chế độ AUTO / MANUAL
    if (msg.indexOf("setMode") >= 0)
    {
      bool newMode = msg.indexOf("true") >= 0;
      applyMode(newMode);
      Serial.println(String("RPC setMode → ") + (newMode ? "AUTO" : "MANUAL"));
    }
    // RPC: Bật / tắt bơm
    else if (msg.indexOf("setWater") >= 0)
    {
      bool newPump = msg.indexOf("true") >= 0;
      applyPump(newPump);
      Serial.println(String("RPC setWater → ") + (newPump ? "ON" : "OFF"));
    }

    // Buzzer báo nhận lệnh thành công
    controlBuzzer(120);
  }

  // Xử lý các thuộc tính khác như dữ liệu mưa (nếu có)
  if (msg.indexOf("\"rain\"") >= 0)
  {
    int rs = msg.indexOf(":\"", msg.indexOf("\"rain\"")) + 2;
    int re = msg.indexOf("\"", rs);
    rainText = msg.substring(rs, re);
    parseRainLevelFromText(rainText);
    Serial.println("Updated rain data: " + rainText);
  }
}

/* ---------------- Reconnect ---------------- */
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Connect TB...");
    if (client.connect("ESP32_Plant", TB_TOKEN, NULL))
    {
      Serial.println("OK");
      digitalWrite(wifiLed, HIGH);
      client.subscribe("v1/devices/me/rpc/request/+");
      client.subscribe("v1/devices/me/attributes");
      String req = "{\"sharedKeys\":\"mode,pump,rain\"}";
      client.publish("v1/devices/me/attributes/request/1", req.c_str());
    }
    else
    {
      Serial.print("rc=");
      Serial.print(client.state());
      Serial.println(" → retry 3s");
      digitalWrite(wifiLed, LOW);
      delay(3000);
    }
  }
}

/* ---------------- Auto watering ---------------- */
void controlMoist()
{
  static unsigned long lastPumpTime = 0;
  static bool pumpActive = false;
  static bool midSent = false;
  static bool cycleSent = false; // thêm cờ chặn gửi trùng
  unsigned long now = millis();

  if (prevMode)
  {
    if (rainLevel >= 2)
    {
      if (togglePump == HIGH)
      {
        Serial.println("⛈ Mưa vừa/to → Tắt bơm");
        applyPump(false);
        controlBuzzer(150);
      }
      return;
    }

    getMoisture();

    // KHi đủ ẩm, chỉ gửi 1 lần
    if (moisturePercentage >= moistPerHigh)
    {
      if (!cycleSent)
      { //  Chưa gửi thì mới gửi
        if (togglePump == HIGH)
        {
          Serial.println("Đủ ẩm → Tắt bơm");
          applyPump(false);
        }
        endMoisture = moisturePercentage;
        sendCycleSummary();
        cycleSent = true; // Đánh dấu đã gửi
      }
      pumpActive = false;
      midSent = false;
      return;
    }

    // Khi đất khô → bắt đầu chu kỳ mới
    if (moisturePercentage < moistPerLow && !pumpActive && rainLevel < 2)
    {
      cycleId++; // tăng chu kỳ ngay khi bắt đầu tưới mới
      Serial.printf("Đất khô → Bắt đầu tưới (Chu kỳ %d)\n", cycleId);

      // gửi lên ThingsBoard để đánh dấu bắt đầu chu kỳ
      String payload = "{\"cycleId\":" + String(cycleId) + "}";
      client.publish("v1/devices/me/telemetry", payload.c_str());
      Serial.println("Sent cycle start marker");

      startMoisture = moisturePercentage;
      cycleStartTime = now;
      pumpActive = true;
      cycleSent = false;
      lastPumpTime = now;
      applyPump(true);
      controlBuzzer(100);
    }

    if (pumpActive)
    {
      unsigned long elapsed = now - lastPumpTime;

      if (togglePump == HIGH && elapsed >= 5000)
      {
        Serial.println("⏸ Dừng bơm 10s");
        applyPump(false);
        lastPumpTime = now;
        midSent = false;
      }
      else if (togglePump == LOW)
      {
        if (!midSent && elapsed >= 5000)
        {
          getMoisture();
          sendTelemetry();
          midSent = true;
        }
        else if (elapsed >= 10000)
        {
          getMoisture();
          if (moisturePercentage >= moistPerHigh)
          {
            if (!cycleSent)
            { // kiểm tra cờ tránh gửi trùng
              endMoisture = moisturePercentage;
              applyPump(false);
              sendCycleSummary();
              cycleSent = true;
            }
            pumpActive = false;
          }
          else
          {
            Serial.println("Vẫn khô → Bơm thêm 5s");
            applyPump(true);
            lastPumpTime = now;
          }
          midSent = false;
        }
      }
    }
  }
  else
  {
    button1.check();
  }
}

/* ---------------- Buttons ---------------- */
void button1Handler(AceButton *, uint8_t eventType, uint8_t)
{
  if (eventType == AceButton::kEventReleased)
    applyPump(!togglePump);
}

void button2Handler(AceButton *, uint8_t eventType, uint8_t)
{
  if (eventType == AceButton::kEventReleased)
    applyMode(!prevMode);
}

/* ---------------- Setup ---------------- */
void setup()
{
  Serial.begin(115200);
  pinMode(wifiLed, OUTPUT);
  pinMode(ModeLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(RelayButtonPin, INPUT_PULLUP);
  pinMode(ModeSwitchPin, INPUT_PULLUP);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PumpPin, PWM_CHANNEL);

  dht.begin();
  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  button1.init(RelayButtonPin);
  button2.init(ModeSwitchPin);

  WiFi.begin(ssid, password);
  Serial.print("🔗 WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  sendTelemetry();
  client.setServer(TB_HOST, TB_PORT);
  client.setCallback(callback);

  randomSeed(analogRead(0));

  controlBuzzer(200);
  digitalWrite(ModeLed, prevMode);
}

/* ---------------- Loop ---------------- */
void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 60000)
  {
    sendTelemetry();
    lastSend = millis();
  }

  button2.check();
  controlMoist();
}
