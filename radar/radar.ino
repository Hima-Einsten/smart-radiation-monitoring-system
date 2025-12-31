#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Servo.h>
#include <math.h>
#include <SoftwareSerial.h>

/* ======================================================
   RS485 CONFIG (SESUAI WIRINGMU)
   RO  -> pin 2
   DI  -> pin 3
   RE+DE -> pin 4
   ====================================================== */
#define RS485_RO     2
#define RS485_DI     3
#define RS485_DE_RE  4
SoftwareSerial rs485(RS485_RO, RS485_DI);

/* ======================================================
   OLED CONFIG
   ====================================================== */
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ======================================================
   PIN CONFIG
   ====================================================== */
#define TRIG_PIN     8
#define ECHO_PIN     9
#define SERVO_PIN    6

#define LED_HIJAU   13
#define LED_MERAH   12
#define BUZZER      11

/* ======================================================
   CONSTANT
   ====================================================== */
const int BATAS_BAHAYA = 15;   // cm
const int MAX_JARAK   = 30;   // cm
const int SERVO_STEP  = 2;

/* ======================================================
   VARIABLE
   ====================================================== */
Servo radarServo;

bool radarAktif = true;
int sudut = 0;
int arah  = 1;
int jarak = 0;

/* ======================================================
   SETUP
   ====================================================== */
void setup() {
  Serial.begin(9600);      // DEBUG
  rs485.begin(9600);       // RS485

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW); // RECEIVE MODE

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  radarServo.attach(SERVO_PIN);

  display.begin(0x3C, true);
  display.clearDisplay();
  display.display();

  tampilRadarOFF();
}

/* ======================================================
   LOOP
   ====================================================== */
void loop() {

  // ===== TERIMA DATA RS485 DARI ESP32 =====
  if (rs485.available()) {
    char cmd = rs485.read();
    if (cmd == '1') radarAktif = true;
    if (cmd == '0') radarAktif = false;
  }

  // ===== MODE RADAR =====
  if (radarAktif) {
    jarak = bacaUltrasonik();
    kontrolAlarm(jarak);
    gerakServo();
    tampilRadarOLED(jarak, sudut);
  } else {
    radarOFF();
  }

  delay(20);
}

/* ======================================================
   ULTRASONIC
   ====================================================== */
int bacaUltrasonik() {
  long durasi;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  durasi = pulseIn(ECHO_PIN, HIGH, 25000);
  if (durasi == 0) return MAX_JARAK;

  return durasi * 0.034 / 2;
}

/* ======================================================
   ALARM & LED
   ====================================================== */
void kontrolAlarm(int d) {
  if (d <= BATAS_BAHAYA) {
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(LED_HIJAU, LOW);
    tone(BUZZER, 1200);
  } else {
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_HIJAU, HIGH);
    noTone(BUZZER);
  }
}

/* ======================================================
   SERVO SWEEP
   ====================================================== */
void gerakServo() {
  radarServo.write(sudut);
  sudut += arah * SERVO_STEP;

  if (sudut >= 180 || sudut <= 0) {
    arah *= -1;
  }
}

/* ======================================================
   RADAR OFF MODE
   ====================================================== */
void radarOFF() {
  radarServo.write(90);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_MERAH, LOW);
  noTone(BUZZER);
  tampilRadarOFF();
}

/* ======================================================
   OLED RADAR DISPLAY (PERSIS CONTOH)
   ====================================================== */
void tampilRadarOLED(int d, int a) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("A:");
  display.print(a);
  display.print((char)247);

  display.setCursor(0, 10);
  display.print("D:");
  display.print(d);
  display.print("cm");

  display.setCursor(80, 0);
  display.print(d <= BATAS_BAHAYA ? "DANGER" : "SAFE");

  int cx = 64;
  int cy = 63;

  for (int r = 15; r <= 45; r += 15) {
    for (int x = 0; x <= 180; x += 5) {
      float rad = radians(x);
      display.drawPixel(
        cx + r * cos(rad),
        cy - r * sin(rad),
        SH110X_WHITE
      );
    }
  }

  float rad = radians(180 - a);
  display.drawLine(
    cx, cy,
    cx + 45 * cos(rad),
    cy - 45 * sin(rad),
    SH110X_WHITE
  );

  if (d > 0 && d <= MAX_JARAK) {
    int objR = map(d, 0, MAX_JARAK, 5, 45);
    display.fillCircle(
      cx + objR * cos(rad),
      cy - objR * sin(rad),
      2,
      SH110X_WHITE
    );
  }

  display.display();
}

/* ======================================================
   OLED RADAR OFF
   ====================================================== */
void tampilRadarOFF() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(20, 20);
  display.print("RADAR");

  display.setCursor(35, 42);
  display.print("OFF");

  display.display();
}
