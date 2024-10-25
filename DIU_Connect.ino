#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// RFID Pins
#define SS_PIN 2
#define RST_PIN 22
#define red 21
#define green 5
#define buzzer 4
#define servoPin 15

String ssid = "demo";
String password = "demo1234";
String base = "https://test-server-iot.vercel.app";       // POST API endpoint

MFRC522 rfid(SS_PIN, RST_PIN);  // Create an instance of the RFID class
Servo servo;                  // Create an instance of the servo class
HTTPClient http;                // Create an HTTPClient object for HTTP requests
double demo_latitude = 23.876762;
double demo_longitude = 90.320480;

void connect_wifi() {
  delay(100);
  Serial.println();
  Serial.print("Connecting ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");  // Print IP address after successful connection
  Serial.println(WiFi.localIP());
}
void pin_setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  servo.attach(servoPin);
  servo.write(0);
}
void on(int pin) {
  digitalWrite(pin, HIGH);
  delay(250);
  digitalWrite(pin, LOW);
}
void open_geat(){
  servo.write(90);
  delay(2000);
  servo.write(0);
}

void setup() {
  Serial.begin(9600);
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Initialize RFID
  // Connect to WiFi
  pin_setup();
  connect_wifi();
}

void loop() {
  // Read RFID card first
  read_rfid();

  // After RFID reading, do WiFi-based tasks
  wifi_tasks();

  delay(1000);  // Add a delay between operations
}

void wifi_tasks() {
  if (WiFi.status() != WL_CONNECTED) {
    connect_wifi();  // Reconnect if WiFi is not connected
  }
  // int gps_patch_status=gps_patch(50.254,62.155);
  Serial.println(gps_put(demo_latitude, demo_longitude));
  demo_latitude += 0.00005;
  demo_longitude += 0.00005;



  // Reinitialize the RFID reader after WiFi operation to ensure it works correctly
  rfid.PCD_Init();  // Reinitialize RFID after WiFi actions
}

int gps_put(double latitude, double longitude) {
  String jsonPayload = "{ \"latitude\": " + String(latitude, 7) + ", \"longitude\": " + String(longitude, 7) + " }";
  return put(base+"/gps", jsonPayload);
}
void print_uuid(String cardUID){
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  Serial.println(cardUID);
}

void read_rfid() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial())
    return;


  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();

  // print_uuid(cardUID);

  String payload = "{\"uid\":\"" + cardUID + "\"}";
  post(base+"/booking", payload);

  rfid.PICC_HaltA();  // Halt the RFID card
}

String get(String endpoint) {
  http.begin(endpoint);
  int res_code = http.GET();
  if (res_code > 0) {
    String response = http.getString();
    return response;
  }
  return "Something went wrong";
}

int post(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int res_status = http.POST(body_data);
  String response = http.getString();
  Serial.println(response);
  on(buzzer);
  if (res_status == 200) {
    on(green);
    open_geat();
  } else {
    on(red);
    if(res_status ==425){
      open_geat();
    }
  }

  return res_status;
}

int put(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  return http.PUT(body_data);
}

int patch(String endpoint, String body_data) {
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  return http.PATCH(body_data);
}
