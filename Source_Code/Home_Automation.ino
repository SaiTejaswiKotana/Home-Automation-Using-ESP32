/*
 * Home Automation Using ESP8266 NodeMCU
 * Based on the source code in the Maven Silicon internship report.
 *
 * Hardware:
 *   ESP8266 NodeMCU, DHT11, LDR module, HC-SR04,
 *   2-channel relay module, fan and light.
 *
 * IMPORTANT:
 *   1. Install the ESP8266 board package in Arduino IDE.
 *   2. Install the DHT sensor library (Adafruit) and Adafruit Unified Sensor.
 *   3. Set WIFI_SSID and WIFI_PASSWORD below before uploading.
 *   4. Verify relay polarity and LDR logic for your modules.
 *   5. The report's code uses a 20 C fan threshold; its test table mentions
 *      28 C. Verify the threshold used in your physical implementation.
 *
 * Safety:
 *   Mains-powered appliances must be wired and tested by a qualified person.
 */

#include <ESP8266WiFi.h>
#include <DHT.h>

// ---------- Wi-Fi credentials ----------
// Enter your own credentials locally. Do not commit real credentials to GitHub.
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ---------- Pin assignments (NodeMCU labels) ----------
#define DHT_PIN       D4
#define DHT_TYPE      DHT11
#define LDR_PIN       D3
#define TRIG_PIN      D1
#define ECHO_PIN      D2
#define RELAY_LIGHT   D5
#define RELAY_FAN     D6

// ---------- Project settings ----------
const float FAN_TEMPERATURE_THRESHOLD_C = 20.0; // Verify: report test table says 28 C.
const float PERSON_DISTANCE_THRESHOLD_CM = 10.0;
const unsigned long SENSOR_INTERVAL_MS = 2000;

// Many relay modules are active LOW. This preserves the inversion
// used by the report's setRelay() function.
const uint8_t RELAY_ON = LOW;
const uint8_t RELAY_OFF = HIGH;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiServer server(80);

bool isManualMode = false;
bool lightIsOn = false;
bool fanIsOn = false;

unsigned long lastSensorUpdate = 0;

// ---------- Relay control ----------
void setLight(bool turnOn) {
  if (lightIsOn != turnOn) {
    digitalWrite(RELAY_LIGHT, turnOn ? RELAY_ON : RELAY_OFF);
    lightIsOn = turnOn;
    Serial.println(turnOn ? F("Light ON") : F("Light OFF"));
  }
}

void setFan(bool turnOn) {
  if (fanIsOn != turnOn) {
    digitalWrite(RELAY_FAN, turnOn ? RELAY_ON : RELAY_OFF);
    fanIsOn = turnOn;
    Serial.println(turnOn ? F("Fan ON") : F("Fan OFF"));
  }
}

// ---------- Sensor readings ----------
float readTemperatureC() {
  float temperature = dht.readTemperature();

  if (isnan(temperature)) {
    Serial.println(F("DHT11 temperature read failed"));
    return NAN;
  }

  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.println(F(" C"));
  return temperature;
}

bool isDark() {
  // The report treats HIGH as dark. Confirm this for your LDR module.
  int state = digitalRead(LDR_PIN);
  bool dark = (state == HIGH);

  Serial.println(dark ? F("It is dark") : F("It is light"));
  return dark;
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Timeout avoids waiting indefinitely if no echo is received.
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    Serial.println(F("Ultrasonic sensor: no echo"));
    return NAN;
  }

  float distance = duration * 0.0343f / 2.0f;
  Serial.print(F("Distance: "));
  Serial.print(distance);
  Serial.println(F(" cm"));
  return distance;
}

// ---------- Automatic mode ----------
void handleAutomaticMode() {
  float distance = readDistanceCm();
  float temperature = readTemperatureC();
  bool dark = isDark();

  bool personPresent =
      !isnan(distance) && (distance <= PERSON_DISTANCE_THRESHOLD_CM);

  if (personPresent) {
    if (!isnan(temperature)) {
      setFan(temperature >= FAN_TEMPERATURE_THRESHOLD_C);
    }

    // Preserve the report's behavior: light turns ON when dark
    // and a person is detected.
    setLight(dark);
  } else {
    setLight(false);
    setFan(false);
  }
}

// ---------- Web page ----------
void sendWebPage(WiFiClient& client) {
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html; charset=utf-8"));
  client.println(F("Connection: close"));
  client.println();

  client.println(F("<!DOCTYPE html><html><head>"));
  client.println(F("<meta name='viewport' content='width=device-width, initial-scale=1'>"));
  client.println(F("<title>NodeMCU Home Automation</title></head><body>"));
  client.println(F("<h2>Home Automation - ESP8266 NodeMCU</h2>"));

  client.print(F("<p>Mode: "));
  client.print(isManualMode ? F("Manual") : F("Automatic"));
  client.println(F("</p>"));

  client.println(F("<p><a href='/?manualOn'><button>Manual Mode</button></a>"));
  client.println(F("<a href='/?manualOff'><button>Automatic Mode</button></a></p>"));

  client.println(F("<h3>Light</h3>"));
  client.println(F("<a href='/?lightOn'><button>ON</button></a>"));
  client.println(F("<a href='/?lightOff'><button>OFF</button></a>"));

  client.println(F("<h3>Fan</h3>"));
  client.println(F("<a href='/?fanOn'><button>ON</button></a>"));
  client.println(F("<a href='/?fanOff'><button>OFF</button></a>"));

  client.println(F("</body></html>"));
}

// ---------- HTTP client handling ----------
void handleClient(WiFiClient& client) {
  String request = client.readStringUntil('\r');
  client.flush();

  Serial.print(F("HTTP request: "));
  Serial.println(request);

  // Mode selection is always available.
  if (request.indexOf("GET /?manualOn") >= 0) {
    isManualMode = true;
  } else if (request.indexOf("GET /?manualOff") >= 0) {
    isManualMode = false;
  } else if (isManualMode) {
    if (request.indexOf("GET /?lightOn") >= 0) {
      setLight(true);
    } else if (request.indexOf("GET /?lightOff") >= 0) {
      setLight(false);
    } else if (request.indexOf("GET /?fanOn") >= 0) {
      setFan(true);
    } else if (request.indexOf("GET /?fanOff") >= 0) {
      setFan(false);
    }
  }

  sendWebPage(client);
  delay(1);
  client.stop();
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(LDR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_LIGHT, RELAY_OFF);
  digitalWrite(RELAY_FAN, RELAY_OFF);

  dht.begin();

  Serial.println();
  Serial.print(F("Connecting to Wi-Fi: "));
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();
  Serial.println(F("Wi-Fi connected"));
  Serial.print(F("Open this address in a browser: http://"));
  Serial.println(WiFi.localIP());

  server.begin();
}

// ---------- Main loop ----------
void loop() {
  WiFiClient client = server.available();

  if (client) {
    handleClient(client);
  }

  if (!isManualMode &&
      (millis() - lastSensorUpdate >= SENSOR_INTERVAL_MS)) {
    lastSensorUpdate = millis();
    handleAutomaticMode();
  }
}
