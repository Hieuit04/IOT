#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

/* ---------------- WiFi ---------------- */
const char *ssid = "Anh em 37";
const char *password = "naptienroicho";

/* ---------------- ThingsBoard MQTT ---------------- */
const char *TB_HOST = "eu.thingsboard.cloud";
const int TB_PORT = 1883;
const char *TB_TOKEN = "JIXUjdNSo5IKcPjlKiQy"; // Token thiết bị của bạn

WiFiClient espClient;
PubSubClient client(espClient);

/* ---------------- Rain Sensor ---------------- */
#define RainPin 34 // Analog pin connected to rain sensor AO

/* ---------------- Variables ---------------- */
String rainText = "No Rain";

/* ---------------- Function Prototypes ---------------- */
void reconnect();
void sendRain();

/* ---------------- Setup ---------------- */
void setup()
{
  Serial.begin(115200);
  pinMode(RainPin, INPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(TB_HOST, TB_PORT);
}

/* ---------------- Reconnect to ThingsBoard ---------------- */
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("🔌 Connecting to ThingsBoard...");
    if (client.connect("ESP32_Rain", TB_TOKEN, NULL))
    {
      Serial.println("Connected!");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" → retry in 3s");
      delay(3000);
    }
  }
}

/* ---------------- Send Rain Data ---------------- */
void sendRain()
{
  int rainVal = analogRead(RainPin);

  if (rainVal > 3500)
    rainText = "No Rain";
  else if (rainVal > 2000)
    rainText = "Light Rain";
  else if (rainVal > 1000)
    rainText = "Moderate Rain";
  else
    rainText = "Heavy Rain";

  // Hiển thị ra Serial Monitor
  Serial.printf("Rain Sensor Value: %d → %s\n", rainVal, rainText.c_str());

  // Chuẩn bị payload JSON
  String payload = "{";
  payload += "\"rain\":\"" + rainText + "\"";
  payload += "}";

  // Gửi lên ThingsBoard
  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Sent to ThingsBoard: " + payload);
}

/* ---------------- Loop ---------------- */
void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) // gửi mỗi 5 giây
  {
    sendRain();
    lastSend = millis();
  }
}