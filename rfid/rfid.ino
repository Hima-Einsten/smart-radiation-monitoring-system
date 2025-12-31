// ======================= //
//   ESP32 RFID + RS485   //
// ======================= //
#include <SPI.h>
#include <RFID.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

//========= WiFi ======================//
char* ssid = "WIFI_SSID";
char* pass = "wifi_password";

//========= RFID UID ==================//
String UID_Access = "243 39 158 52";

//========= MQTT ======================//
char *mqttServer = "broker.emqx.io";
int mqttPort = 1883;
String myClientID = "12324324523";

String Topic_1 = "JogloAtas/data/door";
String Topic_2 = "JogloAtas/data/alarm";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//========= RFID ======================//
#define SS_PIN 5
#define RST_PIN 15
RFID rfid(SS_PIN, RST_PIN);

//========= Hardware ==================//
#define PINRELAY   4
#define PINBUZZER  14
#define LED_RED    32
#define LED_GREEN  33

LiquidCrystal_I2C lcd(0x27, 16, 2);

//========= BUZZER ====================//
#define BUZZER_FREQ 2000

//========= RS485 =====================//
#define RXD2   25
#define TXD2   26
#define RE_DE  27

// ======================= //
//          SETUP         //
// ======================= //
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  SPI.begin();

  pinMode(PINRELAY, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(RE_DE, OUTPUT);

  digitalWrite(PINRELAY, HIGH);
  digitalWrite(RE_DE, LOW); // RECEIVE MODE

  ledcAttach(PINBUZZER, BUZZER_FREQ, 8);
  buzzerOff();

  lcd.init();
  lcd.backlight();

  rfid.init();

  connectToWIFI();
  mqttClient.setServer(mqttServer, mqttPort);
  connect_to_broker(myClientID);

  mqtt_publish(Topic_1, 0);

  buzzerBeep(2, 200);
}

// ======================= //
//           LOOP         //
// ======================= //
void loop() {
  lcd.setCursor(3, 0);
  lcd.print("LAB INSNUK");

  if (rfid.isCard() && rfid.readCardSerial()) {

    String rfidCard = String(rfid.serNum[0]) + " " +
                      String(rfid.serNum[1]) + " " +
                      String(rfid.serNum[2]) + " " +
                      String(rfid.serNum[3]);

    Serial.println(rfidCard);

    // ===== AKSES DITERIMA =====
    if (rfidCard == UID_Access) {
      lcd.clear();
      lcd.print("Akses Diterima");

      digitalWrite(LED_GREEN, HIGH);
      buzzerBeep(2, 200);

      digitalWrite(PINRELAY, LOW);
      mqtt_publish(Topic_1, 1);

      kirimRS485('1');   // RADAR ON

      delay(5000);

      digitalWrite(PINRELAY, HIGH);
      mqtt_publish(Topic_1, 0);

      digitalWrite(LED_GREEN, LOW);
      lcd.clear();
    }

    // ===== AKSES DITOLAK =====
    else {
      lcd.clear();
      lcd.print("Akses Ditolak");

      digitalWrite(LED_RED, HIGH);
      mqtt_publish(Topic_2, 1);

      kirimRS485('0');   // RADAR OFF

      buzzerOn();
      delay(4000);
      buzzerOff();

      mqtt_publish(Topic_2, 0);
      digitalWrite(LED_RED, LOW);
      lcd.clear();
    }

    rfid.halt();
  }

  mqttClient.loop();
}

// ======================= //
//        FUNCTIONS       //
// ======================= //

void kirimRS485(char data) {
  digitalWrite(RE_DE, HIGH);   // TRANSMIT
  delay(5);
  Serial2.write(data);
  Serial2.flush();
  delay(5);
  digitalWrite(RE_DE, LOW);    // RECEIVE
}

void buzzerOn() {
  ledcWriteTone(PINBUZZER, BUZZER_FREQ);
}

void buzzerOff() {
  ledcWriteTone(PINBUZZER, 0);
}

void buzzerBeep(int jumlah, int durasi) {
  for (int i = 0; i < jumlah; i++) {
    buzzerOn();
    delay(durasi);
    buzzerOff();
    delay(150);
  }
}

void connectToWIFI() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

void connect_to_broker(String clientID) {
  while (!mqttClient.connected()) {
    mqttClient.connect(clientID.c_str());
    delay(1000);
  }
}

void mqtt_publish(String topic, int payload) {
  mqttClient.publish(topic.c_str(), String(payload).c_str());
}