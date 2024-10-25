#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <TinyGPS++.h>

// RFID Pins
#define SS_PIN 2
#define RST_PIN 22
#define red 21
#define green 5
#define buzzer 4
#define servoPin 15

// WiFi credentials and base URL
String ssid = "demo";
String password = "demo1234";
String base = "https://test-server-iot.vercel.app"; // POST API endpoint

// Initialize components
MFRC522 rfid(SS_PIN, RST_PIN); // RFID instance
Servo servo;                   // Servo instance
HTTPClient http;               // HTTP client for requests
TinyGPSPlus gps;               // GPS instance
HardwareSerial SerialGPS(2);   // Serial2 for GPS (TX2 on pin 17, RX2 on pin 16)

// Demo latitude and longitude for testing
double _latitude = 23.876762;
double _longitude = 90.320480;

// WiFi connection function
void connect_wifi() {
  delay(100);
  Serial.println();
  Serial.print("Connecting to WiFi ");
  WiFi.begin(ssid, password);

  // Wait until WiFi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address:");
  Serial.println(WiFi.localIP());
}

// Initialize pins and servo
void pin_setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  servo.attach(servoPin);
  servo.write(0);
}

// Function to cpntrol buzzer and led
void on(int pin) {
  digitalWrite(pin, HIGH);
  delay(250);
  digitalWrite(pin, LOW);
}

// Function to open the gate (servo to 90 degrees)
void open_gate() {
  servo.write(90);
  delay(2000);
  servo.write(0);
}

// ESP32 setup function
void setup() {
  Serial.begin(9600);            // Begin serial communication
  SPI.begin();                    // Init SPI bus
  rfid.PCD_Init();                // Initialize RFID
  pin_setup();                    // Setup pins and servo
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17); // Initialize Serial2 for GPS
  connect_wifi();                 // Connect to WiFi
}

// Main loop
void loop() {
  read_rfid();   // Read RFID card data
  wifi_tasks();  // Perform WiFi and GPS tasks
  delay(1000);   // Delay between operations
}

// Function to handle WiFi-based tasks
void wifi_tasks() {
  if (WiFi.status() != WL_CONNECTED) {
    connect_wifi();  // Reconnect if WiFi is not connected
  }

  // Simulate GPS data update
  _latitude += 0.00005;     // This is dummy location update as gps took time to connect when use gps comment out this 
  _longitude += 0.00005;   // and when use dummy location then comment out checkGPS() vice versa

  /////// checkGPS();/////

  gps_put(_latitude, _longitude); // Updating gps data to database

  // Reinitialize RFID reader after WiFi operation
  rfid.PCD_Init();
}

// Send GPS data via HTTP PUT request
int gps_put(double latitude, double longitude) {
  String jsonPayload = "{ \"latitude\": " + String(latitude, 7) + ", \"longitude\": " + String(longitude, 7) + " }";
  return put(base + "/gps", jsonPayload);
}

// Print RFID card UUID
void print_uuid(String cardUID) {
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  Serial.println(cardUID);
}

// Read RFID card and send data to server
void read_rfid() {
  // Check if a new card is present on the sensor
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Retrieve and format the UID
  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
/////////// print_uuid(cardUID);//////////

  // Send UID data to server
  String payload = "{\"uid\":\"" + cardUID + "\"}";
  post(base + "/booking", payload);

  rfid.PICC_HaltA();  // Halt the RFID card
}

// Check and print GPS data
void checkGPS() {
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.location.isValid()) {
        _latitude = gps.location.lat();
        _longitude = gps.location.lng();
        Serial.print(F("Latitude: "));
        Serial.print(gps.location.lat(), 6);
        Serial.print(F(", Longitude: "));
        Serial.println(gps.location.lng(), 6);
      } else {
        Serial.println(F("Waiting for GPS signal..."));
      }
    }
  }
}

// Perform HTTP GET request
String get(String endpoint) {
  http.begin(endpoint);
  int res_code = http.GET();
  if (res_code == 200) {
    return http.getString();
  }
  return "Something went wrong";
}

// Perform HTTP POST request with data
int post(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int res_status = http.POST(body_data);
  String response = http.getString();
  Serial.println(response);
  on(buzzer);

  // Handle response
  if (res_status == 200) {
    on(green);
    open_gate();
  } else {
    on(red);
    if (res_status == 425) {
      open_gate();
    }
  }
  return res_status;
}

// Perform HTTP PUT request with data
int put(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  return http.PUT(body_data);
}

// Perform HTTP PATCH request with data
int patch(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  return http.PATCH(body_data);
}
