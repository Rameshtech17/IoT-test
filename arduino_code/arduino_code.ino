#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#define LED_BUILTIN 2

// =====================================================
// WiFi Configuration
// =====================================================

const char* ssid = "Homeline";
const char* password = "homeline@92";


// =====================================================
// FastAPI Server
// =====================================================

const char* serverUrl =
  "https://io-t-test-ckfj.vercel.app/api/sensors";

const char* controlUrl =
  "https://io-t-test-ckfj.vercel.app/api/device";


// =====================================================
// Device Configuration
// =====================================================

const char* DEVICE_API_KEY =
  "ESP8266_8f72c91";

const char* DEVICE_ID =
  "ESP8266_02";


// =====================================================
// LED Configuration
// =====================================================

// D2 = GPIO4
// const int LED_PIN = D2;


// =====================================================
// Timing
// =====================================================

unsigned long lastControlCheck = 0;
unsigned long lastSensorSend = 0;

const unsigned long CONTROL_INTERVAL = 2000;   // 2 seconds
const unsigned long SENSOR_INTERVAL  = 10000;  // 10 seconds


// =====================================================
// Setup
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP8266 IoT Device");
  Serial.println("=================================");

  // ---------------------------------------------------
  // LED setup
  // ---------------------------------------------------

  pinMode(LED_BUILTIN, OUTPUT);

  // Initially OFF
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("LED initialized -> OFF");


  // ---------------------------------------------------
  // WiFi setup
  // ---------------------------------------------------

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);

  Serial.println("=================================");
}


// =====================================================
// Main Loop
// =====================================================

void loop() {

  // ---------------------------------------------------
  // Check WiFi
  // ---------------------------------------------------

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi disconnected!");

    reconnectWiFi();

    return;
  }


  // ---------------------------------------------------
  // Check LED control status
  // ---------------------------------------------------

  if (millis() - lastControlCheck >= CONTROL_INTERVAL) {

    lastControlCheck = millis();

    getDeviceStatus();
  }


  // ---------------------------------------------------
  // Send sensor data
  // ---------------------------------------------------

  // if (millis() - lastSensorSend >= SENSOR_INTERVAL) {

  //   lastSensorSend = millis();

  //   sendSensorData();
  // }


  delay(100);
}


// =====================================================
// Get LED Status From FastAPI
// =====================================================

void getDeviceStatus() {

  WiFiClientSecure client;

  // For testing HTTPS without certificate validation
  client.setInsecure();

  HTTPClient http;


  // ---------------------------------------------------
  // Build URL
  // ---------------------------------------------------
// "https://io-t-test-ckfj.vercel.app/api/device/ESP8266_02/latest"
  String url =
    String(controlUrl) +
    "/" +
    String(DEVICE_ID)+"/latest";


  Serial.println();
  Serial.println("---------------------------------");
  Serial.println("Checking device control");
  Serial.print("URL: ");
  Serial.println(url);


  // ---------------------------------------------------
  // Start HTTP
  // ---------------------------------------------------

  if (!http.begin(client, url)) {

    Serial.println("ERROR: HTTP begin failed");

    return;
  }


  // ---------------------------------------------------
  // Add API Key
  // ---------------------------------------------------

  http.addHeader(
    "X-API-Key",
    DEVICE_API_KEY
  );


  // ---------------------------------------------------
  // Send GET request
  // ---------------------------------------------------

  int responseCode = http.GET();


  Serial.print("HTTP Response: ");
  Serial.println(responseCode);


  // ---------------------------------------------------
  // Process response
  // ---------------------------------------------------

  if (responseCode == HTTP_CODE_OK) {

    String response = http.getString();

    Serial.print("Server Response: ");
    Serial.println(response);


    // -------------------------------------------------
    // Parse JSON
    // -------------------------------------------------

    DynamicJsonDocument doc(256);

    DeserializationError error =
      deserializeJson(doc, response);


    if (error) {

      Serial.print("JSON Parse Error: ");
      Serial.println(error.c_str());

      http.end();

      return;
    }


    // -------------------------------------------------
    // Get device ID
    // -------------------------------------------------

    const char* deviceId =
      doc["data"]["device_id"] | "";


    // -------------------------------------------------
    // Get LED status
    // -------------------------------------------------

    bool status =
      doc["data"]["status"] | false;


    Serial.print("Device ID: ");
    Serial.println(deviceId);

    Serial.print("Requested LED Status: ");

    if (status) {

      Serial.println("ON");

    } else {

      Serial.println("OFF");
    }


    // -------------------------------------------------
    // Control LED
    // -------------------------------------------------

    if (status) {

      digitalWrite(LED_BUILTIN, HIGH);

      Serial.println("LED -> ON");

    } else {

      digitalWrite(LED_BUILTIN, LOW);

      Serial.println("LED -> OFF");
    }
  }


  // ---------------------------------------------------
  // Unauthorized
  // ---------------------------------------------------

  else if (responseCode == HTTP_CODE_UNAUTHORIZED) {

    Serial.println("ERROR: Invalid API key");
  }


  // ---------------------------------------------------
  // Not Found
  // ---------------------------------------------------

  else if (responseCode == HTTP_CODE_NOT_FOUND) {

    Serial.println("ERROR: Device not found");
  }


  // ---------------------------------------------------
  // Other errors
  // ---------------------------------------------------

  else {

    Serial.print("GET request failed: ");
    Serial.println(responseCode);
  }


  http.end();
}


// =====================================================
// Send Sensor Data
// =====================================================

void sendSensorData() {

  WiFiClientSecure client;

  // For testing HTTPS without certificate validation
  client.setInsecure();

  HTTPClient http;


  Serial.println();
  Serial.println("---------------------------------");
  Serial.println("Sending sensor data");


  // ---------------------------------------------------
  // Start HTTP
  // ---------------------------------------------------

  if (!http.begin(client, serverUrl)) {

    Serial.println("ERROR: HTTP begin failed");

    return;
  }


  // ---------------------------------------------------
  // Headers
  // ---------------------------------------------------

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  http.addHeader(
    "X-API-Key",
    DEVICE_API_KEY
  );


  // ---------------------------------------------------
  // Sensor values
  // ---------------------------------------------------

  float temperature = 36.6;
  float humidity = 38.6;


  // ---------------------------------------------------
  // Create JSON
  // ---------------------------------------------------

  String json = "{";

  json += "\"device_id\":\"";
  json += DEVICE_ID;
  json += "\",";

  json += "\"temperature\":";
  json += String(temperature, 2);
  json += ",";

  json += "\"humidity\":";
  json += String(humidity, 2);

  json += "}";


  Serial.print("Request Body: ");
  Serial.println(json);


  // ---------------------------------------------------
  // POST
  // ---------------------------------------------------

  int responseCode =
    http.POST(json);


  Serial.print("HTTP Response: ");
  Serial.println(responseCode);


  // ---------------------------------------------------
  // Response
  // ---------------------------------------------------

  String response =
    http.getString();

  Serial.print("Server Response: ");
  Serial.println(response);


  // ---------------------------------------------------
  // End HTTP
  // ---------------------------------------------------

  http.end();
}


// =====================================================
// Reconnect WiFi
// =====================================================

void reconnectWiFi() {

  Serial.println("Attempting WiFi reconnect...");

  WiFi.disconnect();

  delay(1000);

  WiFi.begin(ssid, password);


  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 20
  ) {

    delay(500);

    Serial.print(".");

    attempts++;
  }


  Serial.println();


  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi reconnected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi reconnect failed");
  }
}